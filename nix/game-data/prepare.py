"""Select a supported disc image in a fetched source and run the real importer."""

import argparse
from pathlib import Path
import subprocess


def inspect(importer, source):
    return subprocess.run(
        [importer, "--inspect", str(source)], capture_output=True, text=True, check=False
    )


def select_source(importer, source):
    direct = inspect(importer, source)
    if direct.returncode == 0:
        return source
    if not source.is_dir():
        raise ValueError(direct.stderr.strip() or direct.stdout.strip())

    # Prefer descriptors to their raw payloads (CUE/BIN and CCD/IMG).
    paths = sorted(path for path in source.rglob("*") if path.is_file())
    descriptors = [path for path in paths if path.suffix.lower() in {".cue", ".ccd"}]
    images = [path for path in paths if path.suffix.lower() == ".iso"]
    if not descriptors:
        images += [path for path in paths if path.suffix.lower() in {".img", ".bin"}]
    candidates = [path for path in descriptors + images if inspect(importer, path).returncode == 0]
    if len(candidates) == 1:
        return candidates[0]
    if len(candidates) > 1:
        raise ValueError("Several matching disc images; pass the path of one image as source.")
    raise ValueError(
        "No supported game data or disc image found in the source. "
        "Expected version 1.00 files or ISO/IMG/CUE/CCD. "
        "Unpack archives or convert other disc formats before passing them as source.\n"
        + (direct.stderr.strip() or direct.stdout.strip())
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--importer", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--dest", type=Path, required=True)
    args = parser.parse_args()
    try:
        source = select_source(args.importer, args.source)
    except ValueError as exc:
        parser.exit(1, str(exc) + "\n")
    subprocess.run(
        [args.importer, "--import", str(source), "--dest", str(args.dest)], check=True
    )


if __name__ == "__main__":
    main()
