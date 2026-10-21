#!/usr/bin/env python3
"""Stage only browser runtime files for a static host; never publish the build tree."""
import argparse
from pathlib import Path
import shutil

RUNTIME = ('game.html', 'game.js', 'bootstrap.js', 'isolation-worker.js',
           'browser_start.scv', 'saves.js', 'audio-worklet.js', 'titbit.js', 'titbit.wasm')


def package(source, target):
    source, target = Path(source).resolve(), Path(target).resolve()
    if target == source or source in target.parents or target in source.parents:
        raise ValueError('Publication directory must be separate from the build directory')
    missing = [name for name in RUNTIME if not (source / name).is_file()]
    if missing:
        raise FileNotFoundError('Missing runtime files: ' + ', '.join(missing))
    # Refuse to upload leftovers from an unrelated or previous build.
    target.mkdir(parents=True, exist_ok=True)
    if any(target.iterdir()):
        raise ValueError('Publication directory must be empty')
    for name in RUNTIME:
        shutil.copyfile(source / name, target / name)
    shutil.copyfile(source / 'game.html', target / 'index.html')
    (target / '.nojekyll').touch()
    print(f'Browser site: {target}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=Path(__file__).resolve().parents[1] / 'build/web/game')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    package(args.build, args.out)
