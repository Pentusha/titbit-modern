#!/usr/bin/env python3
"""Compile public source recipes as GNU-ABI ELF32; report every translation unit.

This is a feasibility probe, not a game linker. No SDK shims or source rewrites
are synthesized. Logs and objects are isolated under build/gnu-probe/.
"""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def abi_probe(compiler, out):
    source = out/'abi-layout.cpp'
    source.write_text('''class ObservedString { public: char *data; };
#include "lane_engine.h"
EngineObject::EngineObject() : owner(0), field8(0), child(0), extra(0) {}
EngineObject::~EngineObject() {}
int EngineObject::GetType() { return 0; }
ObservedString EngineObject::Export() { ObservedString s; s.data=0; return s; }
void EngineObject::Update(int) {}
void EngineObject::Render(int) {}
int engineObjectSize=sizeof(EngineObject);
int frameSize=sizeof(Frame_0040b090);
''')
    facts = {}
    for name, target in [('gnu', 'i386-pc-linux-gnu'), ('msvc', 'i686-pc-windows-msvc')]:
        command = [compiler, '-target', target, '-fms-extensions', '-DTITBIT_SOURCE_LINK',
                   '-I', str(ROOT/'src/lanes/include'), '-Xclang', '-fdump-vtable-layouts',
                   '-Xclang', '-fdump-record-layouts', '-S', '-emit-llvm', str(source),
                   '-o', str(out/f'abi-{name}.ll')]
        run = subprocess.run(command, capture_output=True, text=True)
        (out/f'abi-{name}.log').write_text(run.stdout+run.stderr)
        facts[name] = {'target': target, 'exitCode': run.returncode,
                       'log': str((out/f'abi-{name}.log').relative_to(ROOT))}
    return facts


def category(output):
    patterns = (
        ('missing-header', r"fatal error: '([^']+)' file not found"),
        ('msvc-intrinsic', r"(?:__asm|__try|__except|__finally|__leave|_Interlocked|_alloca|__emul|__int64)"),
        ('crt-extension', r"(?:_stricmp|_strnicmp|_snprintf|_vsnprintf|_finite|_isnan|_filelength|_fileno|_findfirst|_findnext|_splitpath|_MAX_PATH|_MAX_FNAME|_MAX_EXT)"),
        ('cpp-template', r"(?:typename|template argument|dependent name|undeclared identifier '_Tree'|undeclared identifier '_Rb_tree)"),
        ('type-or-declaration', r"(?:unknown type name|undeclared identifier|does not name a type|no member named)"),
        ('abi-or-layout', r"(?:static assertion|negative size|calling convention|asm label|operator new|operator delete)"),
    )
    return next((name for name, pattern in patterns if re.search(pattern, output)), 'compiler-error')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='clang++-18')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--filter', default='')
    parser.add_argument('--cpp-standard', default='gnu++98')
    parser.add_argument('--c-standard', default='gnu99')
    parser.add_argument('--preserve-win32-defines', action='store_true')
    parser.add_argument('--out', type=Path, default=ROOT/'build/gnu-probe')
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if compiler is None:
        parser.error('Compiler not found: '+args.compiler)
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    compiler_version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
    recipes = json.loads((ROOT/'config/source-build.json').read_text())['recipes']
    recipes = [row for row in recipes if args.filter in row['source']]
    base = [compiler, '-m32', '-fms-extensions', '-fdeclspec', '-DTITBIT_SOURCE_LINK',
            '-O2', '-ffunction-sections', '-fdata-sections', '-ferror-limit=12',
            '-Wno-ignored-attributes', '-Wno-microsoft-cast', '-Wno-pragma-pack']
    started = time.monotonic()

    def compile_one(row):
        source = ROOT/row['source']
        key = row['source'].replace('/', '__')
        result = {'source': row['source'], 'flags': [], 'droppedDefines': []}
        if source.suffix.lower() == '.asm':
            result.update(status='blocked', category='masm-source', reason='MASM syntax requires translation to GNU assembler or C')
            return result
        command = list(base)
        if source.suffix.lower() == '.c':
            command += ['-x', 'c', '-std='+args.c_standard]
        else:
            command += ['-std='+args.cpp_standard]
        for flag in row.get('flags', []):
            if flag.startswith('/D'):
                define = flag[2:]
                if define == 'for=if (0) ; else for' or (not args.preserve_win32_defines and define in ('WIN32', '_WINDOWS')):
                    result['droppedDefines'].append(define)
                else:
                    command += ['-D'+define]
            elif flag == '/Zp4':
                command += ['-fpack-struct=4']
        for folder in [source.parent, ROOT/'src/lanes/include']+[ROOT/p for p in row.get('includes', [])]:
            command += ['-I', str(folder)]
        output = args.out/(key+'.o')
        command += ['-c', str(source), '-o', str(output)]
        result['flags'] = command[1:]
        try:
            run = subprocess.run(command, cwd=ROOT/row.get('cwd', '.'), capture_output=True, text=True, errors='replace', timeout=90)
            diagnostics = run.stdout+run.stderr
            (args.out/(key+'.log')).write_text(diagnostics)
            result['exitCode'] = run.returncode
            if run.returncode == 0:
                result.update(status='compiled', object=str(output.relative_to(ROOT)),
                              sha256=hashlib.sha256(output.read_bytes()).hexdigest())
            else:
                errors = [line.strip() for line in diagnostics.splitlines() if ': error:' in line or ': fatal error:' in line]
                result.update(status='blocked', category=category('\n'.join(errors)), errors=errors)
        except subprocess.TimeoutExpired:
            result.update(status='blocked', category='timeout')
        return result

    results = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for future in as_completed([pool.submit(compile_one, row) for row in recipes]):
            row = future.result()
            results.append(row)
            print(f"[{len(results)}/{len(recipes)}] {row['status']}: {row['source']}", flush=True)
    counts = Counter(row.get('category', 'compiled') for row in results)
    headers = Counter()
    for row in results:
        for line in row.get('errors', []):
            match = re.search(r"fatal error: '([^']+)' file not found", line)
            if match:
                headers[match.group(1)] += 1
    report = {'compiler': compiler_version, 'target': 'i386-pc-linux-gnu / ELF32 / Itanium C++ ABI',
              'sourceLink': True, 'compat': False, 'cppStandard': args.cpp_standard, 'cStandard': args.c_standard,
              'seconds': round(time.monotonic()-started, 2),
              'translationUnits': len(results), 'summary': dict(counts), 'missingHeaders': dict(headers.most_common()),
              'rows': sorted(results, key=lambda row: row['source']), 'abiProbe': abi_probe(compiler, args.out)}
    (args.out/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'rows'}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
