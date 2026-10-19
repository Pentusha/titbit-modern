"""Build editable resource source with SDK RC and historical CVTRES."""
import hashlib
import os
import pathlib
import re
import shutil

import match


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resource_compiler():
    found = shutil.which('rc.exe')
    if found:
        return pathlib.Path(found)
    kits = pathlib.Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Windows Kits/10/bin'
    versions = sorted((p for p in kits.glob('*') if re.fullmatch(r'\d+(?:\.\d+)+', p.name)),
                      key=lambda p: tuple(map(int, p.name.split('.'))), reverse=True)
    for version in versions:
        for architecture in ('x86', 'x64'):
            compiler = version / architecture / 'rc.exe'
            if compiler.is_file():
                return compiler
    raise FileNotFoundError('Installed Windows SDK resource compiler rc.exe not found')


def compile_resources(out, invoke):
    source = match.BASE / 'src/lanes/codex/resources/titbit.rc'
    compiler = resource_compiler()
    converter = match.BASE / 'toolchain/msvc6.5pp/Bin/CVTRES.EXE'
    short_out = pathlib.Path(match.short_path(out))
    res, obj = out / 'game_resources.res', out / 'game_resources.obj'
    rc = [str(compiler), '/nologo', '/fo' + str(short_out / res.name), source.name]
    cvt = [match.short_path(converter), '/NOLOGO', '/MACHINE:IX86',
           '/OUT:' + str(short_out / obj.name), str(short_out / res.name)]
    if invoke(rc, source.parent, out / 'resources-rc.log'):
        raise RuntimeError('Resource source compilation failed; see resources-rc.log')
    if invoke(cvt, out, out / 'resources-cvtres.log'):
        raise RuntimeError('Resource COFF conversion failed; see resources-cvtres.log')
    inputs = [source, *sorted(source.parent.glob('*.ico'))]
    return {'sourceInputs': [{'path': str(p), 'sha256': digest(p)} for p in inputs],
            'resourceCompiler': str(compiler), 'resourceCompilerSha256': digest(compiler),
            'converter': str(converter), 'converterSha256': digest(converter),
            'commands': [rc, cvt], 'object': str(obj), 'objectSha256': digest(obj),
            'resource': str(res), 'resourceSha256': digest(res)}
