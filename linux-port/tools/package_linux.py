#!/usr/bin/env python3
"""Package public Linux sources, native ELF and local setup/Studio launchers."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile
from export_public import excluded

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT,help='Clean exported Linux source tree')
    p.add_argument('--executable',type=Path,default=ROOT/'build/titbit')
    p.add_argument('--tools',type=Path,default=ROOT/'build/titbit-tools',
                   help='Import wizard + Studio executable from tools/build_tools.sh; skipped if missing')
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--version',required=True)
    p.add_argument('--commit',required=True)
    a=p.parse_args();source=a.source.resolve();binary=a.executable.read_bytes()
    if binary[:7]!=b'\x7fELF\x01\x01\x01' or int.from_bytes(binary[18:20],'little')!=3:
        raise ValueError('Expected native ELF32/i386 executable')
    original_digest=hashlib.sha256(binary).hexdigest()
    # Keep .eh_frame and C++ exception tables; discard only build-time DWARF
    # paths and compiler comments. The source executable is never modified.
    with tempfile.TemporaryDirectory(prefix='titbit-release-') as temporary:
        stripped=Path(temporary)/'titbit'
        subprocess.run(['strip','--strip-debug','--remove-section=.comment','-o',str(stripped),str(a.executable.resolve())],check=True)
        binary=stripped.read_bytes()
    archive_root='Titbit-'+a.version+'-linux-x86'
    files={}
    for path in sorted(source.rglob('*')):
        relative=path.relative_to(source)
        if not path.is_file() or path.is_symlink() or excluded(relative) or relative.name=='Start Linux.cmd':continue
        payload=path.read_bytes()
        if payload[:4]==b'\x7fELF' or payload[:2]==b'MZ':raise ValueError('Unexpected binary: '+str(relative))
        if path.suffix=='.sh' or path.name=='Makefile':payload=payload.replace(b'\r\n',b'\n')
        files[relative.as_posix()]=payload
    files['titbit']=binary
    # A Git checkout shares the portable ABI layer with the Windows build.
    # Standalone Linux packages/exports carry that same source layer locally.
    portable=source/'portable' if (source/'portable').is_dir() else source.parent/'portable'
    for path in sorted(portable.rglob('*')):
        if path.is_file() and not path.is_symlink():
            relative=Path('portable')/path.relative_to(portable)
            if not excluded(relative):files[relative.as_posix()]=path.read_bytes()
    if a.tools.is_file():
        tools=a.tools.read_bytes()
        if tools[:5]!=b'\x7fELF\x02':raise ValueError('Expected x86-64 ELF titbit-tools')
        files['titbit-tools']=tools
        licenses=a.tools.parent/'tools-licenses'
        if not licenses.is_dir():raise ValueError('Missing '+str(licenses)+': rebuild with tools/build_tools.sh')
        for path in sorted(licenses.iterdir()):
            files['docs/licenses/runtime/'+path.name]=path.read_bytes()
    metadata={'version':a.version,'sourceCommit':a.commit,'target':'ELF32 i386; tested Ubuntu 24.04 x86-64 / WSL2',
              'gameAssetsIncluded':False,'sdkIncluded':False,
              'sourceBuildExecutableSHA256':original_digest,
              'files':{name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in files.items()}}
    files['package.json']=(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n').encode()
    a.out.parent.mkdir(parents=True,exist_ok=True)
    with a.out.open('wb') as raw, gzip.GzipFile(fileobj=raw,mode='wb',filename='',mtime=0) as zipped, tarfile.open(fileobj=zipped,mode='w') as archive:
        for name,payload in files.items():
            entry=tarfile.TarInfo(archive_root+'/'+name);entry.size=len(payload)
            entry.mode=0o755 if name in ('titbit','titbit-tools') or name.endswith('.sh') else 0o644
            entry.mtime=0;archive.addfile(entry,io.BytesIO(payload))
    digest=hashlib.sha256(a.out.read_bytes()).hexdigest()
    a.out.with_name(a.out.name+'.sha256').write_text(digest+'  '+a.out.name+'\n')
    print(json.dumps({'archive':str(a.out),'sha256':digest,'files':len(files),'bytes':a.out.stat().st_size}))

if __name__=='__main__':main()
