#!/usr/bin/env python3
"""Export the Linux source tree without local SDKs, assets or build products."""
import argparse
import fnmatch
import json
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[1]
DIRECTORIES=('config','docs','editor','platform','probes','src','third_party','tools')
FILES=('README.md','.gitignore','requirements-studio.txt','start-game.sh','Start Linux.cmd','THIRD_PARTY_NOTICES.md')
RUNTIME=('crt_*','stl_*','eh_*','iostream_*','cpp_eh*','locale_*','msvcrt_*')
ARTIFACTS={'.o','.obj','.a','.so','.exe','.dll','.lib','.pdb','.pyc','.ppm','.ll','.bc','.map','.log','.pid','.sav','.ezd','.avi','.iso','.img','.cue','.ccd','.sub','.dmp','.meat','.eza','.tga','.cab','.hdr'}

def excluded(relative):
    parts=relative.parts
    if any(p in ('__pycache__','out','build','dist','stage','.git','.venv','projects') for p in parts):return True
    if relative.suffix.lower() in ARTIFACTS:return True
    if relative.name=='XTREE' or relative.name.startswith('.env'):return True
    if fnmatch.fnmatch(relative.name,'icon_*.ico'):return True
    if relative.as_posix().startswith(('src/crt/','src/scripts/','src/script_headers/')):return True
    if len(parts)>1 and parts[0]=='third_party' and (parts[1] in ('msvc600-reference','nt5-crt-reference') or parts[1].startswith('opennt-')):return True
    if parts[:3]==('src','lanes','codex') and any(fnmatch.fnmatch(relative.name,p) for p in RUNTIME):return True
    return False

def export(destination):
    destination=destination.resolve()
    if destination==ROOT or ROOT in destination.parents:raise ValueError('Export outside the working Linux source directory.')
    if destination.exists() and any(destination.iterdir()):raise ValueError('Destination must be empty: '+str(destination))
    destination.mkdir(parents=True,exist_ok=True)
    copied=[]
    for name in DIRECTORIES:
        for source in sorted((ROOT/name).rglob('*')):
            if not source.is_file() or source.is_symlink():continue
            relative=source.relative_to(ROOT)
            if excluded(relative):continue
            with source.open('rb') as stream:magic=stream.read(4)
            if magic==b'\x7fELF' or magic[:2]==b'MZ':continue
            target=destination/relative;target.parent.mkdir(parents=True,exist_ok=True)
            if target.suffix=='.sh' or target.name=='Makefile':
                target.write_text(source.read_text(encoding='utf-8').replace('\r\n','\n'),encoding='utf-8',newline='\n')
            else:shutil.copy2(source,target)
            copied.append(relative.as_posix())
    for name in FILES:
        source=ROOT/name
        if source.exists():
            target=destination/name
            if target.suffix=='.sh':target.write_text(source.read_text(encoding='utf-8'),encoding='utf-8',newline='\n')
            else:shutil.copy2(source,target)
            copied.append(name)
    return copied

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dest',required=True,type=Path)
    args=parser.parse_args()
    copied=export(args.dest)
    print(json.dumps({'directory':str(args.dest),'sourceFiles':len(copied)},ensure_ascii=False))

if __name__=='__main__':main()
