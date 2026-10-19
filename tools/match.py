"""Fast per-function matching loop for any lane.

  python tools/match.py src/lanes/claude/foo.cpp [--flags "/O2 /G5 ..."] [--asm 0x401950]

Compiles one source with the historical VC6 compiler, links every function
COMDAT whose symbol resolves to a known original function start, and compares
it byte-for-byte with titbit.exe.

Symbol -> address resolution, in order:
  1. bindings from every analysis/*-module.json (already proven names),
  2. lanes/symbols.json (names declared by lane workers),
  3. any symbol whose name contains an 8-digit address `_00XXXXXX`
     (FUN_00401950, ?Update_00412340@Scene@@QAEXXZ, g_00502478 ...).

Unbound relocation targets are reported and, for the comparison only, filled
with the value the original expects at that spot ("assumed"). A function is
MATCH only when every relocation was bound; MATCH* means "bytes equal, but
some relocations were assumed" - add the bindings, then rerun.
This is a development check. Publication still goes through `verify`
(dashboard/progress.mjs) with the linked bytes this tool writes.

Optional <source stem>.bindings.json beside the source supplies translation-unit
bindings without modifying lanes/symbols.json. Its "bindings" map is scoped to
that source; "unboundSymbols" suppresses disproven inherited hypotheses. Record
the independent address evidence in the sidecar's "evidence" field. This does
not relax relocation or byte-comparison requirements.
"""
import argparse, ctypes, glob, hashlib, json, os, pathlib, re, struct, subprocess, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from coff_link import CoffObject

BASE = pathlib.Path(__file__).resolve().parent.parent
ROOT = BASE.parent
EXE = BASE/'original/game/App_Executables/titbit.exe'
DEFAULT_FLAGS = '/O2 /G5 /Ob1 /Gy /Zl /GR- /GX'
ADDR_RE = re.compile(r'_00([0-9a-fA-F]{6})(?![0-9a-fA-F])')


def short_path(p):
    buf = ctypes.create_unicode_buffer(4096)
    return buf.value if ctypes.windll.kernel32.GetShortPathNameW(str(p), buf, 4096) else str(p)


IMPORTS = {}


def load_image():
    import pefile
    pe = pefile.PE(str(EXE))
    for d in pe.DIRECTORY_ENTRY_IMPORT:
        for s in d.imports:
            if s.name:
                IMPORTS[s.name.decode()] = s.address
    return pe.get_memory_mapped_image(), pe.OPTIONAL_HEADER.ImageBase


def load_inventory():
    inv = json.loads((BASE/'analysis/baseline/function-inventory.json').read_text())
    return {int(f['address'], 16): f for f in inv['functions']}


def load_bindings(source=None):
    config = BASE/'config/source-build.json'
    out = {k:int(v,16) for k,v in json.loads(config.read_text(encoding='utf-8'))['bindings'].items()} if config.exists() else {}
    for path in glob.glob(str(BASE/'analysis/*-module.json')):
        try:
            b = json.loads(pathlib.Path(path).read_text(encoding='utf-8')).get('bindings') or {}
        except Exception:
            continue
        for k, v in b.items():
            if isinstance(v, str) and v.startswith('0x'):
                out.setdefault(k, int(v, 16))
    lane = BASE/'lanes/symbols.json'
    if lane.exists():
        for k, v in json.loads(lane.read_text(encoding='utf-8')).items():
            out[k] = int(v, 16) if isinstance(v, str) else v
    # A lane-owned source sidecar keeps overloaded SDK names and their evidence
    # local to this translation unit without editing the shared merge ledger.
    # Explicit per-unit keys retain address_of() precedence.
    if source is not None:
        sidecar = pathlib.Path(source).with_suffix('.bindings.json')
        if sidecar.exists():
            local = json.loads(sidecar.read_text(encoding='utf-8'))
            for k in local.get('unboundSymbols', []):
                # Suppress an inherited, independently disproven hypothesis.
                # address_of() stops at this scoped key instead of falling
                # through to the shared global binding.
                out[f'{pathlib.Path(source).stem}:{k}'] = None
            for k, v in local['bindings'].items():
                address = int(v, 16) if isinstance(v, str) else v
                if not isinstance(address, int) or not 0 <= address <= 0xffffffff:
                    raise ValueError(f'Invalid source binding: {sidecar}: {k}')
                out[f'{pathlib.Path(source).stem}:{k}'] = address
    return out


UNIT = ''   # source file stem; per-unit bindings are written "stem:symbol"


def address_of(name, bindings):
    if f'{UNIT}:{name}' in bindings:
        return bindings[f'{UNIT}:{name}']
    if name in bindings:
        return bindings[name]
    if name.startswith('__imp__'):
        return IMPORTS.get(name[7:].split('@')[0])
    if '@?1?' in name:
        # A function-local static contains its owner's address, not its own.
        return None
    if name.startswith('??_') or name.startswith('??1'):
        # Destructors too: by convention a class name carries its CONSTRUCTOR address.
        # vtables, deleting dtors, RTTI...: the address in the class name is
        # not theirs. Bind explicitly in lanes/symbols.json.
        return None
    # In a C++ member name, an address in its CLASS denotes that class's
    # constructor, not every method. Only the first decorated-name component
    # may supply an implicit address: ??0Class_00... for its constructor, or
    # ?Method_00... for an independently addressed method. Qualified class and
    # template argument addresses must never bind an unrelated virtual method.
    m = ADDR_RE.search(name.split('@', 1)[0] if name.startswith('?') else name)
    return int(m.group(1), 16) | 0 if m else None


def lane_allows(lane, address):
    if lane == 'codex':
        return 0x45c000 <= address < 0x4d0000
    if lane == 'engine42':
        return (0x420000 <= address < 0x430000 or 0x440000 <= address < 0x45c2c0
                or 0x4c0000 <= address < 0x4d0000)
    if lane == 'engine41':
        return 0x410000 <= address < 0x420000 or 0x430000 <= address < 0x440000 or 0x4c0000 <= address < 0x4d0000
    if lane == 'engine40':
        return 0x400000 <= address < 0x410000 or 0x4c0000 <= address < 0x4d0000
    if lane == 'claude':
        return 0x400000 <= address < 0x410000 or 0x4c0000 <= address < 0x4d0000
    return True


def body_bytes(fn, image, base):
    return b''.join(image[int(r['start'], 16)-base:int(r['end'], 16)-base+1]
                    for r in sorted(fn['ranges'], key=lambda r: int(r['start'], 16)))


def disasm(code, address, n=40):
    try:
        import capstone
    except ImportError:
        return ['(capstone not available)']
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return [f'{i.address:08x}  {i.bytes.hex():<20} {i.mnemonic} {i.op_str}' for i in md.disasm(code, address)][:n]


def ret_forms(code, address):
    """Set of return forms ('' / '0xN'): the callee-cleanup ABI."""
    try:
        import capstone
    except ImportError:
        return set()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return {i.op_str for i in md.disasm(bytes(code), address) if i.mnemonic == 'ret'}


_literal_cache = {}


def locate_literal(c, s, image, base):
    """Bind a compiler literal ($SG..., ??_C@...) by its own bytes: the object's
    data section content must occur exactly once in the original data sections."""
    if not (s['name'].startswith('$SG') or s['name'].startswith('??_C@')):
        return None
    sec = c.sections[s['section']]
    if sec['flags'] & 0x20 or sec['relocations']:
        return None
    blob = sec['bytes'][s['value']:]
    if s['name'].startswith('$SG'):
        blob = blob[:blob.find(b'\0')+1] if b'\0' in blob else blob
    if len(blob) < 4:
        return None
    key = bytes(blob)
    if key not in _literal_cache:
        lo = 0xcd000  # .rdata onwards; code never holds literals in this EXE
        hits, at = [], image.find(key, lo)
        while at != -1 and len(hits) < 2:
            hits.append(at); at = image.find(key, at+1)
        _literal_cache[key] = base+hits[0] if len(hits) == 1 else None
    return _literal_cache[key]


def prove_local(c, s, r, at, addend, address, bindings, image, base, proven, stack):
    """A symbol defined in another section of this object (EH handler stubs in
    .text$x, funcinfo tables in .xdata$x, unwind funclets...). Infer where the
    original placed it from the original operand, then accept that address only
    if the WHOLE compiled section, linked at that address, equals the original
    bytes there (recursively for what it references)."""
    idx = s['section']
    want = struct.unpack_from('<I', image, address-base+at)[0]
    value = want if r['kind'] == 0x6 else (want+address+at+4) & 0xffffffff
    sym = (value-addend) & 0xffffffff
    start = sym-s['value']
    if idx in proven:
        return sym if proven[idx] == start else None
    if idx in stack:
        return sym
    sec = c.sections[idx]
    if not sec['size'] or start < base or start-base+sec['size'] > len(image):
        return None
    if not (sec['flags'] & 0x40000000):  # must be readable initialized content
        return None
    proven[idx] = start
    code, assumed = link(c, idx, start, bindings, image, base, proven, stack)
    if assumed or code != image[start-base:start-base+len(code)]:
        del proven[idx]
        return None
    return sym


def link(c, sec_index, address, bindings, image, base, proven=None, stack=None):
    """Like CoffObject.linked_section, but records unbound symbols instead of failing."""
    proven = {} if proven is None else proven
    stack = set() if stack is None else stack
    stack.add(sec_index)
    section = c.sections[sec_index]
    code = bytearray(section['bytes'])
    assumed = []
    for r in section['relocations']:
        s, at = r['symbol'], r['offset']
        # Source-only COFF links have no library search. Resolve the compiler's
        # weak fallback only when no strong native binding exists. In particular
        # VC6 vtables name a weak vector-deleting dtor whose emitted fallback is
        # the scalar-deleting dtor. Its section still needs the normal full proof.
        weak_seen = set()
        while (address_of(s['name'], bindings) is None
               and s.get('weakSearch') in (2, 3)
               and s.get('weakFallback') in c.by_name
               and s['name'] not in weak_seen):
            weak_seen.add(s['name'])
            s = c.by_name[s['weakFallback']]
        addend = struct.unpack_from('<I', code, at)[0]
        if s['section'] == sec_index:
            target = address+s['value']
        else:
            target = address_of(s['name'], bindings)
        if target is None and s['section'] > 0:
            target = locate_literal(c, s, image, base)
        if target is None and s['section'] > 0 and address-base+at+4 <= len(image):
            target = prove_local(c, s, r, at, addend, address, bindings, image, base, proven, stack)
        if target is None:
            want = struct.unpack_from('<I', image, address-base+at)[0]
            exp = (want+address+at+4) & 0xffffffff if r['kind'] == 0x14 else want
            assumed.append((s['name'], hex((exp-addend) & 0xffffffff)))
            struct.pack_into('<I', code, at, want)
            continue
        value = target+addend if r['kind'] == 0x6 else target+addend-(address+at+4)
        struct.pack_into('<I', code, at, value & 0xffffffff)
    stack.discard(sec_index)
    return bytes(code), assumed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('source')
    ap.add_argument('--flags', default=DEFAULT_FLAGS)
    ap.add_argument('--compiler', default='msvc6.5pp')
    ap.add_argument('--asm', help='print side-by-side disassembly for this address')
    ap.add_argument('--json', action='store_true')
    a = ap.parse_args()
    # Git Bash (MSYS) rewrites a leading "/O2" into "C:/Program Files/Git/O2"; CL then
    # silently compiles WITHOUT optimisation. Undo that rewrite for every switch.
    fixed = re.sub(r'(?<![\w/])[A-Za-z]:/Program Files/Git/', '/', a.flags)
    if fixed != a.flags:
        print('note: repaired MSYS-mangled flags:', fixed, file=sys.stderr)
        a.flags = fixed

    src = pathlib.Path(a.source).resolve()
    global UNIT
    UNIT = src.stem
    out = BASE/'build/lanes'/src.parent.name
    out.mkdir(parents=True, exist_ok=True)
    obj = out/(src.stem+'.obj')
    if src.suffix.lower() == '.asm':
        cl = pathlib.Path(os.environ['TITBIT_VC6'])/'Bin/ml.exe'
        if a.flags == DEFAULT_FLAGS:
            a.flags = '/coff /Zm /Cx /DI386 /D_WIN32 /D_MT /D_CRTBLD'
        cmd = [short_path(cl), '/nologo', '/c', *a.flags.split(), '/Sa',
               f'/Fl{obj.with_suffix(".lst")}', f'/Fo{obj}', str(src)]
    else:
        cl = pathlib.Path(os.environ['TITBIT_VC6'])/'Bin/CL.EXE'
        inc = [f'/I{cl.parent.parent/"Include"}', f'/I{cl.parent.parent/"ATL/Include"}', f'/I{BASE/"src/lanes/include"}']
        cmd = [short_path(cl), '/nologo', '/c', *a.flags.split(), *inc, '/FAcs', f'/Fa{obj.with_suffix(".asm")}', f'/Fo{obj}', str(src)]
    run = subprocess.run(cmd, cwd=src.parent, capture_output=True, text=True, errors='replace')
    if run.returncode:
        print(run.stdout+run.stderr)
        sys.exit(2)

    image, base = load_image()
    inv = load_inventory()
    bindings = load_bindings(src)
    c = CoffObject(obj)
    results = []
    proven_all = {}
    for s in c.symbols.values():
        if s['section'] <= 0 or not (s['type'] & 0x20) or s['value'] != 0 or s['storage'] not in (2, 3):
            continue
        addr = address_of(s['name'], bindings)
        if addr not in inv or not lane_allows(src.parent.name, addr):
            results.append({'symbol': s['name'], 'status': 'UNMAPPED'})
            continue
        fn = inv[addr]
        orig = body_bytes(fn, image, base)
        code, assumed = link(c, s['section'], addr, bindings, image, base, proven_all)
        # VC6 pads COMDATs; compare the function body length only, but a longer
        # compiled body (real code past the original end) is a mismatch.
        size_ok = len(code.rstrip(b'\xcc\x90')) <= len(orig) <= len(code)
        cmp = code[:len(orig)]
        # Ghidra bodies can exclude a compiler switch table or contain split
        # ranges. Only project them after comparing the COMPLETE compiler
        # section at its real native location; never append target bytes.
        ranges = sorted(fn['ranges'], key=lambda r: int(r['start'], 16))
        inside = all(0 <= int(r['start'],16)-addr <= int(r['end'],16)-addr < len(code) for r in ranges)
        whole_section_equal = code == image[addr-base:addr-base+len(code)]
        if inside and whole_section_equal:
            cmp = b''.join(code[int(r['start'],16)-addr:int(r['end'],16)-addr+1] for r in ranges)
            size_ok = True
        diff = [i for i in range(min(len(cmp), len(orig))) if cmp[i] != orig[i]]
        same = size_ok and not diff and len(cmp) == len(orig)
        # No target-derived relocation value can be promoted to a strict match,
        # including compiler-local EH labels. Bind those before publishing.
        hard = assumed
        st = ('MATCH' if not hard else 'MATCH*') if same else 'DIFF'
        abi = bool(ret_forms(orig, addr)) and ret_forms(orig, addr) != ret_forms(code[:len(code.rstrip(bytes([0xcc, 0x90])))], addr)
        bin_path = out/f'{addr:08x}-{hashlib.sha256(cmp).hexdigest()[:16]}.bin'
        bin_path.write_bytes(cmp)
        results.append({'abiMismatch': abi, 'symbol': s['name'], 'address': hex(addr), 'status': st, 'origSize': len(orig),
                        'compiledSize': len(code.rstrip(b'\xcc\x90')), 'diffBytes': len(diff),
                        'firstDiff': hex(diff[0]) if diff else None, 'assumed': assumed,
                        'wholeSectionEqual': whole_section_equal,
                        'compiled': str(bin_path.relative_to(ROOT))})
        if a.asm and int(a.asm, 16) == addr:
            left, right = disasm(orig, addr, 400), disasm(cmp, addr, 400)
            for i in range(max(len(left), len(right))):
                l = left[i] if i < len(left) else ''
                r = right[i] if i < len(right) else ''
                mark = '  ' if l[10:] == r[10:] else '!='
                print(f'{l:<60}{mark} {r[10:]}')
    # Funclets (EH handlers/unwind actions) proven byte-equal as whole sections
    # are functions of their own in the inventory (Unwind@..., 0x4c....).
    done = {int(r['address'], 16) for r in results if r.get('address')}
    for idx, start in proven_all.items():
        sec = c.sections[idx]
        if not sec['flags'] & 0x20:
            continue
        code, unbound = link(c, idx, start, bindings, image, base, dict(proven_all))
        if unbound or code != image[start-base:start-base+len(code)]:
            continue
        for faddr, fn in inv.items():
            if not lane_allows(src.parent.name, faddr) or faddr in done or not (start <= faddr < start+len(code)):
                continue
            rs = sorted(fn['ranges'], key=lambda r: int(r['start'], 16))
            if not all(start <= int(r['start'], 16) and int(r['end'], 16) < start+len(code) for r in rs):
                continue
            body = b''.join(code[int(r['start'], 16)-start:int(r['end'], 16)-start+1] for r in rs)
            if body != body_bytes(fn, image, base):
                continue
            bin_path = out/f'{faddr:08x}-{hashlib.sha256(body).hexdigest()[:16]}.bin'
            bin_path.write_bytes(body)
            done.add(faddr)
            results.append({'symbol': f'{sec["name"]}#{idx}:{fn["name"]}', 'address': hex(faddr), 'status': 'MATCH',
                            'origSize': len(body), 'compiledSize': len(body), 'diffBytes': 0, 'firstDiff': None,
                            'assumed': [], 'compiled': str(bin_path.relative_to(ROOT))})
    # A native body may contain a tail-called helper from another complete
    # compiler section. Project only from fully relocated, fully compared CODE
    # sections; sparse gaps and target bytes are never reconstruction inputs.
    code_locations = dict(proven_all)
    for symbol in c.symbols.values():
        if symbol['section'] <= 0 or symbol['value'] or not symbol['type'] & 0x20:
            continue
        location = address_of(symbol['name'], bindings)
        if location is not None and lane_allows(src.parent.name, location):
            code_locations[symbol['section']] = location
    verified_code = []
    for idx, start in code_locations.items():
        if not c.sections[idx]['flags'] & 0x20:
            continue
        raw, unbound = link(c, idx, start, bindings, image, base, dict(proven_all))
        if not unbound and raw == image[start-base:start-base+len(raw)]:
            verified_code.append((start, raw))
    already = {int(r['address'],16) for r in results if r.get('address') and r['status']=='MATCH'}
    for faddr, fn in inv.items():
        if not lane_allows(src.parent.name, faddr) or faddr in already or not any(start <= faddr < start+len(raw) for start,raw in verified_code):
            continue
        pieces=[]
        for part in sorted(fn['ranges'], key=lambda r:int(r['start'],16)):
            cursor=int(part['start'],16); end=int(part['end'],16)+1
            while cursor < end:
                provider=next(((start,raw) for start,raw in verified_code if start <= cursor < start+len(raw)),None)
                if provider is None:
                    break
                start,raw=provider; stop=min(end,start+len(raw))
                pieces.append(raw[cursor-start:stop-start]); cursor=stop
            if cursor != end:
                break
        else:
            body=b''.join(pieces)
            if body != body_bytes(fn,image,base):
                continue
            destination=out/f'{faddr:08x}-{hashlib.sha256(body).hexdigest()[:16]}.bin'
            destination.write_bytes(body)
            previous=next((r for r in results if r.get('address')==hex(faddr)),None)
            row={'symbol': previous['symbol'] if previous else 'source_sections:'+fn['name'],
                 'address':hex(faddr),'status':'MATCH','origSize':len(body),
                 'compiledSize':len(body),'diffBytes':0,'firstDiff':None,'assumed':[],
                 'wholeSectionsEqual':True,'compiled':str(destination.relative_to(ROOT))}
            if previous is not None:
                previous.clear(); previous.update(row)
            else:
                results.append(row)

    # Source coverage ledger per lane: every function that has compiling source,
    # exact or not. tools/coverage.py summarises it.
    ledger = BASE/f'lanes/status-{src.parent.name}.json'
    led = json.loads(ledger.read_text(encoding='utf-8')) if ledger.exists() else {}
    source_key = src.relative_to(ROOT).as_posix()
    # Recompiling a changed/renamed source replaces its previous rows: obsolete
    # symbol hypotheses must not keep contributing to source coverage forever.
    # A reassigned address range does not erase earlier compiled sources from
    # other units. Newly generated rows still pass lane_allows() above; this
    # recompile replaces only this unit's observations.
    led = {key: row for key, row in led.items() if row.get('source') != source_key}
    for r in results:
        if r['status'] != 'UNMAPPED':
            led[r['address']] = {'status': r['status'], 'diffBytes': r['diffBytes'], 'size': r['origSize'],
                                 'source': source_key, 'compiler': a.compiler, 'flags': a.flags}
    ledger.write_text(json.dumps(led, indent=0, sort_keys=True), encoding='utf-8')
    if a.json:
        print(json.dumps(results, indent=1))
        return
    for r in results:
        if r['status'] == 'UNMAPPED':
            print(f"UNMAPPED  {r['symbol']}")
            continue
        extra = '' if r['status'] == 'MATCH' else f" diff={r['diffBytes']} first={r['firstDiff']} size {r['compiledSize']}/{r['origSize']}"
        if r.get('abiMismatch'):
            extra += '  !! ABI: ret N differs from original (calling convention/argument count) - will crash the game'
        print(f"{r['status']:<7} {r['address']}  {r['symbol']}{extra}")
        for name, want in r['assumed']:
            print(f"          assumed {name} -> {want}")


if __name__ == '__main__':
    main()
