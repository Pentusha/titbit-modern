"""Rename Microsoft decorated IR symbols before producing ELF.

ELF linkers interpret '@' as symbol-version syntax. Merely recompiling unchanged
MSVC LLVM IR can silently bind Derived::method to Base::method. Keep a reversible
mapping, including COMDAT names, rather than replacing punctuation with '_'.
This does NOT translate Windows exception-handling IR or provide its runtime.
"""
import argparse
import json
from pathlib import Path
import re


def convert(text):
    names = {}
    def name(match):
        prefix, raw = match.groups()
        if '@' not in raw and '?' not in raw and '\\' not in raw:
            return match.group(0)
        encoded = 'titbit_ms_' + raw.encode('utf-8').hex()
        names[raw] = encoded
        return prefix + encoded
    text=re.sub(r'([@$])"([^"\n]+)"', name, text)
    # ELF has only Any/NoDeduplicate selection. The build report retains the
    # original selections so the final linker can pick largest table providers.
    text=re.sub(r'(= comdat )(?:largest|samesize|exactmatch)\b',r'\1any',text)
    # /DEFAULTLIB and other Windows SDK #pragma comment directives are not ELF
    # linker options. Linux dependencies are supplied by the native linker step.
    text=re.sub(r'^!llvm\.linker\.options = .*\n','',text,flags=re.MULTILINE)
    # SysV i386 libc requires the caller's stack to be 16-byte aligned, unlike
    # the historical Microsoft 4-byte default. The driver backend flag alone
    # does not affect an existing IR module: Clang stores it as module metadata.
    if '"override-stack-alignment"' in text:
        text=re.sub(r'(!"override-stack-alignment", i32 )\d+',r'\g<1>16',text)
    else:
        number=max((int(n) for n in re.findall(r'^!(\d+) =',text,re.MULTILINE)),default=-1)+1
        flag=f'!{number}'
        found=re.search(r'^!llvm\.module\.flags = !\{([^}]*)\}',text,re.MULTILINE)
        if found:
            values=found.group(1)
            text=text[:found.start(1)]+values+(', ' if values else '')+flag+text[found.end(1):]
        else:
            text+='\n!llvm.module.flags = !{'+flag+'}\n'
        text+=f'\n!{number} = !{{i32 1, !"override-stack-alignment", i32 16}}\n'
    # SeeR can invoke native member callbacks from a packed argument stack. Keep
    # entry realignment as well, so those callees can safely call SysV helpers.
    text=re.sub(r'^(attributes #\d+ = \{)([^\n]*)(\})',
                lambda m:m.group(0) if '"stackrealign"' in m.group(2)
                else m.group(1)+m.group(2)+' "stackrealign" '+m.group(3),text,flags=re.MULTILINE)
    return text, names


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path)
    p.add_argument('dest',type=Path)
    args=p.parse_args()
    text,names=convert(args.source.read_text())
    args.dest.write_text(text)
    args.dest.with_suffix('.symbols.json').write_text(json.dumps(names,indent=2))
