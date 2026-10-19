#!/usr/bin/env python3
"""Copy the licence texts of everything PyInstaller packed into titbit-tools.

  python3 tools/collect_tool_licenses.py WORKDIR SITE_PACKAGES OUT

Reads WORKDIR/titbit-tools/Analysis-00.toc, maps every bundled system library
to its Debian package (dpkg -S) and copies /usr/share/doc/<package>/copyright;
Pillow and PyInstaller ship their licences in their dist-info folders.
"""
import ast
from pathlib import Path
import shutil
import subprocess
import sys


def bundled_files(toc_path):
    toc = ast.literal_eval(toc_path.read_text())
    files = set()
    def walk(node):
        if isinstance(node, (list, tuple)):
            if len(node) == 3 and all(isinstance(x, str) for x in node) and node[2] in ('BINARY', 'EXTENSION'):
                files.add(node[1])
            else:
                for item in node:
                    walk(item)
    walk(toc)
    return sorted(files)


def main(argv):
    work, site, out = map(Path, argv[1:4])
    out.mkdir(parents=True, exist_ok=True)
    packages, unmapped = set(), []
    for path in bundled_files(work / 'titbit-tools' / 'Analysis-00.toc'):
        if path.startswith(str(site)):
            continue                      # wheel contents: covered by dist-info licences below
        result = subprocess.run(['dpkg', '-S', str(Path(path).resolve())], capture_output=True, text=True)
        if result.returncode:
            result = subprocess.run(['dpkg', '-S', path], capture_output=True, text=True)
        if result.returncode:
            unmapped.append(path)
            continue
        packages.add(result.stdout.split(':', 1)[0])
    for package in sorted(packages):
        copyright = Path('/usr/share/doc') / package / 'copyright'
        if copyright.is_file():
            shutil.copy2(copyright, out / f'{package}-copyright')
    for pattern in ('pillow-*.dist-info', 'pyinstaller-*.dist-info'):
        for info in site.glob(pattern):
            for p in [*info.glob('licenses/*'), *info.glob('LICENSE*'), *info.glob('COPYING*')]:
                if p.is_file():
                    shutil.copy2(p, out / f"{info.name.split('-')[0]}-{p.name}")
    if unmapped:
        raise SystemExit('No package found for bundled files: ' + ', '.join(unmapped))
    print(f'{len(list(out.iterdir()))} licence files from {len(packages)} packages -> {out}')


if __name__ == '__main__':
    main(sys.argv)
