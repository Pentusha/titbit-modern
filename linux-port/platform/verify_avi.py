#!/usr/bin/env python3
"""Compare the native VfW adapter against independent FFmpeg CLI decoding.

Only test inputs are read; no game data is copied into the source tree.
"""
import argparse
import json
import pathlib
import subprocess
import os
import tempfile


def fnv64(data):
    result = 14695981039346656037
    for value in data:
        result = ((result ^ value) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return f"{result:016x}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("game", type=pathlib.Path)
    parser.add_argument("--report", type=pathlib.Path)
    args = parser.parse_args()
    videos = sorted(p for p in args.game.iterdir() if p.suffix.lower() == ".avi")
    if not videos:
        raise SystemExit("No AVI test inputs found")
    binary = pathlib.Path(__file__).resolve().with_name("avi-smoke")
    dump = tempfile.TemporaryDirectory(prefix="titbit-avi-reference-")
    result = subprocess.run([str(binary), *map(str, videos)], check=True, capture_output=True, text=True,
                            env={**os.environ, "TITBIT_AVI_TEST_DUMP": dump.name})
    native = {}
    frame_count = 0
    for line in result.stdout.splitlines():
        if line.startswith("FRAME "):
            path, position, geometry, digest = line[6:].rsplit(" ", 3)
            native[(pathlib.Path(path).name, int(position))] = digest
            frame_count += 1
    checks = []
    for video in videos:
        reference = subprocess.run([
            "ffmpeg", "-v", "error", "-i", str(video), "-an", "-vf", "vflip",
            "-frames:v", "1", "-pix_fmt", "bgra", "-f", "rawvideo", "pipe:1",
        ], check=True, capture_output=True).stdout
        expected = fnv64(reference)
        actual = native[(video.name, 0)]
        pixels = (pathlib.Path(dump.name) / (video.name + ".bgra")).read_bytes()
        if len(pixels) != len(reference):
            raise AssertionError(f"{video.name}: pixel-buffer length mismatch")
        maximum_error = max(abs(a - b) for a, b in zip(pixels, reference))
        average_error = sum(abs(a - b) for a, b in zip(pixels, reference)) / len(reference)
        if maximum_error > 3 or average_error > 0.25:
            raise AssertionError(f"{video.name}: excessive decoder difference: max={maximum_error}, mean={average_error}")
        streams = json.loads(subprocess.run([
            "ffprobe", "-v", "error", "-show_entries", "stream=codec_type,codec_name",
            "-of", "json", str(video),
        ], check=True, capture_output=True, text=True).stdout)["streams"]
        checks.append({"file": video.name, "firstFrameFnv64": actual, "referenceFnv64": expected,
                       "exactPixels": actual == expected, "maxChannelError": maximum_error,
                       "meanChannelError": average_error, "streams": streams})
    report = {"ok": True, "videos": len(videos), "nativeFrameChecks": frame_count,
              "independentFirstFrameComparisons": len(checks), "checks": checks}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    exact = sum(check["exactPixels"] for check in checks)
    dump.cleanup()
    print(f"PASS {len(videos)} AVI files, {frame_count} native frame/DIB checks, "
          f"{exact}/{len(checks)} byte-identical independent FFmpeg first frames; "
          f"maximum channel difference {max(check['maxChannelError'] for check in checks)}")


if __name__ == "__main__":
    main()
