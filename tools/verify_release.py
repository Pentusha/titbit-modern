"""Check that a release ZIP holds exactly what this source tree builds.

  python tools/verify_release.py TitbitModern-<version>-windows-x64.zip [--build build/modern-portable]

1. Every file in the ZIP matches SHA256SUMS.txt (the archive is intact).
2. TitbitModern.exe and TitbitSettings.exe match your own build byte for byte.
   Build first from the same commit (see package.json "sourceCommit"):
     git checkout <sourceCommit>
     python tools/build_modern.py --fresh
   If they differ, compiler/linker and runtime library hashes are compared with
   package.json "reproducible". Historical VC6 packages remain supported.
TitbitSetup.exe and TitbitStudio.exe are PyInstaller bundles, not byte-reproducible;
they are built from tools/titbit_setup.py and editor/ of the same commit.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import sys
import zipfile

BASE = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('zip', type=Path)
    parser.add_argument('--build', type=Path, default=BASE / 'build/modern-portable')
    args = parser.parse_args()
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')
    with zipfile.ZipFile(args.zip) as archive:
        names = [n for n in archive.namelist() if not n.endswith('/')]
        root = PurePosixPath(names[0]).parts[0]
        read = lambda name: archive.read(f'{root}/{name}')
        package = json.loads(read('package.json'))
        executable_hashes = {name: sha(read(name)) for name in ('TitbitModern.exe', 'TitbitSettings.exe')}
        sums = dict(reversed(line.split('  ', 1)) for line in read('SHA256SUMS.txt').decode('utf-8').splitlines() if line)
        bad = [n for n, h in sums.items() if sha(read(n)) != h]
        extra = sorted({str(PurePosixPath(n).relative_to(root)) for n in names} - set(sums) - {'SHA256SUMS.txt'})
    print(f"Package {package['version']}, source commit {package['sourceCommit']}")
    print('1. Archive vs SHA256SUMS.txt:', 'OK' if not bad and not extra else f'MISMATCH {bad} unlisted {extra}')
    ok = not bad and not extra
    info_path = args.build / 'build-info.json'
    if not info_path.is_file():
        print(f'2. No local build in {args.build}: run tools/build_modern.py --fresh first.')
        return 1
    for name in ('TitbitModern.exe', 'TitbitSettings.exe'):
        mine = sha((args.build / name).read_bytes())
        same = mine == executable_hashes[name] == package['executables'][name]
        ok &= same
        print(f"2. {name}: {'identical to the release' if same else 'DIFFERENT'} ({mine[:16]}…)")
    if not ok:
        info = json.loads(info_path.read_text(encoding='utf-8'))
        expected = package.get('reproducible') or {}
        for name, digest in (expected.get('tools') or expected.get('vc6Tools') or {}).items():
            if info.get('tools', {}).get(name) != digest:
                print(f'   your tool {name} differs from the release build')
        for name, digest in (expected.get('headers') or {}).items():
            if info.get('headers', {}).get(name) != digest:
                print(f'   your headers {name} differ from the release build')
        theirs = {l['name']: l['sha256'] for l in (expected.get('libraries') or expected.get('vc6Libraries') or [])}
        for lib in info.get('libraries', []):
            if lib['name'] in theirs and theirs[lib['name']] != lib['sha256']:
                print(f"   your runtime library {lib['name']} differs from the release build")
        if info.get('sourceDateEpoch') != expected.get('sourceDateEpoch'):
            print('   different commit time: build from the commit in package.json, or set SOURCE_DATE_EPOCH='
                  + str(expected.get('sourceDateEpoch')))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
