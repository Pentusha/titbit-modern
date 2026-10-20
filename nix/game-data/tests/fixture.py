"""Generate an ISO and 19 tiny files; contains no original game assets."""

import hashlib
import json
from pathlib import Path
import sys


def both(value, width=4):
    return value.to_bytes(width, "little") + value.to_bytes(width, "big")


def record(name, extent, size, directory=False):
    length = 33 + len(name) + (not len(name) % 2)
    result = bytearray(length)
    result[0] = length
    result[2:10] = both(extent)
    result[10:18] = both(size)
    result[25] = 2 if directory else 0
    result[28:32] = both(1, 2)
    result[32] = len(name)
    result[33:33 + len(name)] = name
    return result


def main():
    root = Path(sys.argv[1])
    (root / "files").mkdir(parents=True)
    (root / "torrent").mkdir()
    payloads = {"QUEST.ezd": b"synthetic archive"}
    payloads.update({f"clip{i:02}.avi": bytes([i]) * (20 + i) for i in range(18)})
    manifest = {"version": 1, "files": {}}
    data = bytearray(64 * 2048)
    pvd = bytearray(2048)
    pvd[:7] = b"\x01CD001\x01"
    pvd[80:88] = both(64)
    pvd[128:132] = both(2048, 2)
    root_record = record(b"\0", 20, 2048, True)
    pvd[156:156 + len(root_record)] = root_record
    data[16 * 2048:17 * 2048] = pvd
    data[17 * 2048:17 * 2048 + 7] = b"\xffCD001\x01"
    entries = [root_record, record(b"\1", 20, 2048, True)]
    for extent, (name, payload) in enumerate(payloads.items(), 21):
        (root / "files" / name).write_bytes(payload)
        manifest["files"][name] = {
            "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest()
        }
        entries.append(record((name.upper() + ";1").encode(), extent, len(payload)))
        data[extent * 2048:extent * 2048 + len(payload)] = payload
    directory = b"".join(entries)
    data[20 * 2048:20 * 2048 + len(directory)] = directory
    (root / "torrent" / "disc.iso").write_bytes(data)
    clonecd = root / "clonecd"
    clonecd.mkdir()
    raw = bytearray()
    for offset in range(0, len(data), 2048):
        raw += b"\0" + b"\xff" * 10 + b"\0\0\0\0\1"
        raw += data[offset:offset + 2048] + bytes(288)
    (clonecd / "IMAGE.img").write_bytes(raw)
    (clonecd / "IMAGE.cue").write_text(
        'FILE "IMAGE.img" BINARY\n TRACK 1 MODE1/2352\n INDEX 1 00:00:00\n'
    )
    (clonecd / "IMAGE.CCD").write_text(
        "[Disc]\nSessions=1\nDataTracksScrambled=0\n[TRACK 1]\nMODE=1\nINDEX 1=0\n"
    )
    (root / "assets.json").write_text(json.dumps(manifest))


if __name__ == "__main__":
    main()
