#!/usr/bin/env python3
"""Validate the actual PCM emitted by SDL's disk audio driver."""
import argparse
import array
import json
import os
import pathlib
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--report", type=pathlib.Path)
args = parser.parse_args()
binary = pathlib.Path(__file__).resolve().with_name("audio-capture-smoke")
checks = []
with tempfile.TemporaryDirectory(prefix="titbit-pcm-") as directory:
    for volume, expected in ((65535, 10000), (32768, 5000), (0, 0)):
        output = pathlib.Path(directory) / f"volume-{volume}.raw"
        result = subprocess.run([str(binary), str(volume)], check=True, capture_output=True, text=True,
                                env={**os.environ, "SDL_AUDIODRIVER": "disk",
                                     "SDL_DISKAUDIOFILE": str(output), "SDL_DISKAUDIODELAY": "5"})
        samples = array.array("h", output.read_bytes())
        if sys.byteorder != "little":
            samples.byteswap()
        peak = max(map(abs, samples), default=-1)
        if peak != expected:
            raise AssertionError(f"volume={volume}: peak={peak}, expected={expected}")
        checks.append({"volume": volume, "pcmBytes": output.stat().st_size, "peak": peak})
report = {"ok": True, "driver": "SDL disk", "checks": checks}
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("PASS actual SDL PCM output: full volume 10000, half volume 5000, silence 0")
