"""Compile and run the source ABI regression tests against the current Windows CRT.

No game executable, original game data, VC6 headers or VC6 libraries are used.
The test is a console process and does not create a game or settings window.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from build_windows import BASE, digest, toolchain


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path)
    parser.add_argument('--mingw-headers',type=Path)
    parser.add_argument('--vc-tools',type=Path)
    parser.add_argument('--windows-sdk',type=Path)
    parser.add_argument('--sdk-version')
    parser.add_argument('--out',type=Path,default=BASE/'build/windows-runtime-test')
    args=parser.parse_args()
    compiler,mingw,vc,sdk,tools,resource=toolchain(args)
    out=args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    environment=os.environ.copy()
    environment['PATH']=str(tools)+os.pathsep+str(compiler.parent)+os.pathsep+environment.get('PATH','')
    environment.pop('INCLUDE',None)
    environment.pop('LIB',None)
    coverage=['fixed string/vector/deque/XML object layouts','overlapping string assignment and replacement',
              'vector self insertion and ownership','deque growth at both ends and FIFO order',
              'map/multimap copying and lookup','list removal','typed exceptions and exception copying',
              'uncaught_exception during stack unwinding','C locale facets','C++ file streams and seeking',
              'native UCRT stdio/getc/ungetc','fread/feof state','descriptor/file length','64-bit fpos round trip',
              'Win32 multimedia/input and ATL record sizes and offsets']
    report={'ok':False,'requiresVC6':False,'target':'PE32 / Intel 80386 / Microsoft object ABI',
            'runtime':'current MSVC libcmt + libvcruntime + Windows SDK libucrt','coverage':coverage,
            'compiler':str(compiler),'steps':[]}

    def save():
        (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

    def command(name,arguments,cwd=BASE,timeout=120):
        result=subprocess.run(arguments,cwd=cwd,env=environment,capture_output=True,timeout=timeout)
        log=out/(name+'.log')
        log.write_bytes(result.stdout+result.stderr)
        report['steps'].append({'stage':name,'exitCode':result.returncode,'log':str(log),
                                'command':list(map(str,arguments))})
        save()
        if result.returncode:
            raise RuntimeError(name+' failed: '+(result.stdout+result.stderr).decode('utf-8',errors='replace')[-6000:])
        return result

    try:
        base=[str(compiler),'-target','i686-pc-windows-msvc','-fms-extensions','-fms-compatibility',
              '-fms-compatibility-version=19.44','-fgnuc-version=4.2.1','-fdeclspec','-fdelayed-template-parsing',
              '-nostdinc','-DTITBIT_PORTABLE_HEADERS','-DTITBIT_WINDOWS_UCRT','-DTITBIT_PORTABLE_SMOKE',
              '-D_USE_32BIT_TIME_T','-D__MINGW32__','-D__i386__','-D_X86_',
              '-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections',
              '-Wno-ignored-attributes','-Wno-pragma-pack','-Wno-microsoft-cast','-Wno-macro-redefined',
              '-Wno-implicit-function-declaration','-c']
        for directory in (BASE/'portable/include/win32',BASE/'portable/include/cxx',BASE/'portable/include/crt',resource,mingw):
            base += ['-I',str(directory)]
        sources=['portable/src/container_smoke.cpp','portable/src/win32_abi.cpp',
                 'linux-port/src/linux_runtime/tests/vc6_cpp_smoke.cpp','portable/src/windows_runtime.cpp',
                 'linux-port/src/linux_runtime/tests/smoke_main.c']
        objects=[]
        for index,name in enumerate(sources):
            source=BASE/name
            obj=out/(source.stem+'.obj')
            flags=['-x','c','-std=gnu89'] if source.suffix=='.c' else ['-std=c++98','-fcxx-exceptions','-fexceptions']
            command('compile-'+source.stem,[*base,*flags,'-o',str(obj),source.name],cwd=source.parent)
            objects.append(obj)
        executable=out/'smoke.exe'
        libraries=[vc/'lib/x86'/name for name in ('libcmt.lib','libvcruntime.lib','legacy_stdio_definitions.lib',
                                                  'legacy_stdio_wide_specifiers.lib')]
        libraries += [sdk/'ucrt/x86/libucrt.lib',sdk/'um/x86/kernel32.lib']
        command('link',[str(tools/'link.exe'),'/nologo','/Brepro','/NODEFAULTLIB','/MACHINE:X86','/OPT:REF',
                        '/SUBSYSTEM:CONSOLE','/ENTRY:mainCRTStartup','/OUT:'+str(executable),
                        *map(str,objects),*map(str,libraries)])
        work=out/'work'
        (work/'build').mkdir(parents=True,exist_ok=True)
        result=command('execute',[str(executable)],cwd=work,timeout=30)
        report.update(ok=True,exitCode=0,stdout=result.stdout.decode('utf-8',errors='replace'),
                      stderr=result.stderr.decode('utf-8',errors='replace'),executableSHA256=digest(executable))
    except (OSError,RuntimeError,subprocess.TimeoutExpired) as error:
        report['error']=str(error)
    save()
    print(json.dumps({key:value for key,value in report.items() if key!='steps'},ensure_ascii=False,indent=2))
    return 0 if report['ok'] else 1


if __name__=='__main__':
    sys.exit(main())
