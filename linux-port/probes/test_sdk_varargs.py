"""Verify the installed VC98 stdarg adapter against native ELF32 vsnprintf."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'probes')]
from compile_ms_ir import sdk_overlay
from retarget_ir import convert


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vc98',type=Path,default=os.environ.get('TITBIT_VC6'))
    p.add_argument('--out',type=Path,default=ROOT/'probes/out/sdk-varargs')
    a=p.parse_args()
    if not a.vc98 or not (a.vc98/'Include').is_dir():p.error('--vc98 requires an installed VC98 SDK')
    sdk=a.vc98.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
    overlay=sdk_overlay(sdk,out)
    source=ROOT/'probes/msabi_varargs.cpp'
    ir=out/'varargs.ll'
    commands=[['clang++-18','--target=i386-pc-windows-msvc','-O2','-std=c++98',
        '-fms-extensions','-fms-compatibility','-fms-compatibility-version=12.0',
        '-Wno-nonportable-include-path','-ivfsoverlay',str(overlay),'-I',str(sdk/'Include'),
        '-S','-emit-llvm',str(source),'-o',str(ir)]]
    for command in commands:subprocess.run(command,check=True)
    text=ir.read_text()
    if 'call void @llvm.va_start' not in text:raise AssertionError('SDK macro did not use LLVM va_start')
    safe,_=convert(text);(out/'safe.ll').write_text(safe)
    commands=[
        ['clang++-18','--target=i386-pc-windows-msvc-elf','-O2','-Wno-override-module',
         '-c',str(out/'safe.ll'),'-o',str(out/'source.o')],
        ['gcc','-m32','-c',str(ROOT/'probes/varargs_main.c'),'-o',str(out/'main.o')],
        ['clang++-18','-m32','-no-pie',str(out/'main.o'),str(out/'source.o'),'-o',str(out/'verify')]]
    for command in commands:subprocess.run(command,check=True)
    subprocess.run([str(out/'verify')],check=True,timeout=10)


if __name__=='__main__':main()
