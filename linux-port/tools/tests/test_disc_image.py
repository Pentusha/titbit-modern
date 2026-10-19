"""Disc reader checks; optional real-disc checks use TITBIT_TEST_ISO/RAW/FILES."""
import hashlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from disc_image import DiscImage, DiscImageError


def both(value, width=4):
    return value.to_bytes(width, "little") + value.to_bytes(width, "big")


def record(name, extent, size, directory=False):
    length = 33 + len(name) + (not len(name) % 2)
    result = bytearray(length)
    result[0] = length
    result[2:10] = both(extent)
    result[10:18] = both(size)
    result[18:25] = bytes([126, 10, 19, 12, 34, 56, 0])
    result[25] = 2 if directory else 0
    result[28:32] = both(1, 2)
    result[32] = len(name)
    result[33:33 + len(name)] = name
    return result


def iso_data(extra_root=()):
    data = bytearray(32 * 2048)
    pvd = bytearray(2048)
    pvd[:7] = b"\x01CD001\x01"
    pvd[80:88] = both(32)
    pvd[128:132] = both(2048, 2)
    root = record(b"\0", 20, 2048, True)
    pvd[156:156 + len(root)] = root
    data[16 * 2048:17 * 2048] = pvd
    data[17 * 2048:17 * 2048 + 7] = b"\xffCD001\x01"
    root_records = [root, record(b"\1", 20, 2048, True),
                    record(b"DATA", 21, 2048, True),
                    record(b"README.TXT;1", 22, 12), *extra_root]
    tree = b"".join(root_records)
    data[20 * 2048:20 * 2048 + len(tree)] = tree
    child = b"".join([record(b"\0", 21, 2048, True),
                       record(b"\1", 20, 2048, True),
                       record(b"PAYLOAD.BIN;1", 23, 5000),
                       record(b"EMPTY.;1", 26, 0)])
    data[21 * 2048:21 * 2048 + len(child)] = child
    data[22 * 2048:22 * 2048 + 12] = b"hello world\n"
    payload = bytes(range(256)) * 19 + bytes(range(136))
    data[23 * 2048:23 * 2048 + len(payload)] = payload
    return data


def raw_data(data):
    output = bytearray()
    for offset in range(0, len(data), 2048):
        header = b"\0" + b"\xff" * 10 + b"\0" + b"\0\0\0\1"
        output += header + data[offset:offset + 2048] + bytes(288)
    return output


class DiscImageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, data, name="test.iso"):
        path = self.root / name
        path.write_bytes(data)
        return path

    def test_iso_and_raw_inventory_and_bounded_files(self):
        expected = bytes(range(256)) * 19 + bytes(range(136))
        for data, name in [(iso_data(), "disc.iso"), (raw_data(iso_data()), "disc.img")]:
            with self.subTest(name=name), DiscImage(self.write(data, name)) as disc:
                self.assertEqual(set(disc.entries), {"readme.txt", "data/payload.bin", "data/empty."})
                self.assertEqual(disc.entries["data/payload.bin"].name, "DATA/PAYLOAD.BIN")
                with disc.open("Data/Payload.Bin") as stream:
                    self.assertEqual(stream.read(99999), expected)
                    self.assertEqual(stream.read(), b"")
                    self.assertEqual(stream.seek(-3100, io.SEEK_END), 1900)
                    self.assertEqual(stream.read(2500), expected[1900:4400])
                    self.assertEqual(stream.seek(0), 0)
                    target = bytearray(17)
                    self.assertEqual(stream.readinto(target), 17)
                    self.assertEqual(target, expected[:17])
                    for position in (-1, 5001):
                        with self.assertRaises(ValueError):
                            stream.seek(position)
                self.assertEqual(disc.open("DATA/EMPTY.").read(), b"")
                with self.assertRaises(FileNotFoundError):
                    disc.open("missing")
                for unsafe in ("../readme.txt", "/README.TXT", "data/../readme.txt", "data\\payload.bin"):
                    with self.assertRaises(DiscImageError):
                        disc.open(unsafe)

    def test_parallel_file_positions_and_closed_parent(self):
        disc = DiscImage(self.write(iso_data()))
        first, second = disc.open("readme.txt"), disc.open("readme.txt")
        self.assertEqual(first.read(5), b"hello")
        self.assertEqual(second.read(), b"hello world\n")
        self.assertEqual(first.read(), b" world\n")
        disc.close()
        with self.assertRaises(ValueError):
            first.read(1)
        with self.assertRaises(ValueError):
            disc.open("readme.txt")
        first.close()
        with self.assertRaises(ValueError):
            first.tell()

    def test_cue_and_ccd(self):
        self.write(raw_data(iso_data()), "disc.img")
        cue = self.write(b'FILE "disc.img" BINARY\n TRACK 1 MODE1/2352\n INDEX 1 00:00:00\n', "disc.cue")
        ccd = self.write(b"[Disc]\nSessions=1\nDataTracksScrambled=0\n[TRACK 1]\nMODE=1\nINDEX 1=0\n", "disc.ccd")
        for path in (cue, ccd):
            with DiscImage(path) as disc:
                self.assertEqual(disc.open("readme.txt").read(), b"hello world\n")

    def test_reject_unsafe_or_ambiguous_cue(self):
        for text in [
            'FILE "../disc.img" BINARY\nTRACK 1 MODE1/2352\nINDEX 1 00:00:00',
            'FILE "C:\\disc.img" BINARY\nTRACK 1 MODE1/2352\nINDEX 1 00:00:00',
            'FILE "disc.img" BINARY\nTRACK 1 MODE1/2352\nINDEX 1 00:02:00',
            'FILE "disc.img" BINARY\nTRACK 1 MODE1/2352\nINDEX 1 00:00:00\nTRACK 2 AUDIO',
            'FILE "disc.img" BINARY\nFILE "other.img" BINARY\nTRACK 1 MODE1/2352\nINDEX 1 00:00:00',
        ]:
            with self.subTest(text=text), self.assertRaises(DiscImageError):
                DiscImage(self.write(text.encode(), "disc.cue"))

    def test_reject_bad_geometry_and_truncation(self):
        cases = [iso_data()[:-1], raw_data(iso_data())[:-1], bytearray(2048)]
        data = iso_data()
        data[16 * 2048 + 80:16 * 2048 + 88] = both(33)
        cases.append(data)
        data = iso_data()
        data[16 * 2048 + 128:16 * 2048 + 132] = both(1024, 2)
        cases.append(data)
        data = iso_data()
        data[16 * 2048 + 84] ^= 1
        cases.append(data)
        for index, data in enumerate(cases):
            with self.subTest(index=index), self.assertRaises(DiscImageError):
                DiscImage(self.write(data))

    def test_reject_cycles_collisions_bad_paths_and_extents(self):
        bad_records = [record(b"LOOP", 20, 2048, True),
                       record(b"data", 26, 0), record(b"README.TXT;2", 26, 0),
                       record(b"../bad", 26, 0), record(b"EVIL.BIN;1", 31, 4096)]
        for bad in bad_records:
            with self.subTest(name=bad[33:]), self.assertRaises(DiscImageError):
                DiscImage(self.write(iso_data([bad])))

    def test_reject_invalid_raw_sector_when_file_read(self):
        data = raw_data(iso_data())
        data[23 * 2352 + 15] = 2
        with DiscImage(self.write(data, "disc.img")) as disc:
            with self.assertRaises(DiscImageError):
                disc.open("data/payload.bin").read()


def digest_file(stream):
    digest = hashlib.sha256()
    while chunk := stream.read(1024 * 1024):
        digest.update(chunk)
    return digest.hexdigest()


@unittest.skipUnless(os.environ.get("TITBIT_TEST_ISO") and os.environ.get("TITBIT_TEST_RAW"),
                     "Set TITBIT_TEST_ISO and TITBIT_TEST_RAW for original-disc checks")
class OriginalDiscTests(unittest.TestCase):
    def test_entire_iso_raw_and_extracted_tree(self):
        files = Path(os.environ["TITBIT_TEST_FILES"]) if os.environ.get("TITBIT_TEST_FILES") else None
        extracted = {p.relative_to(files).as_posix().lower(): p
                     for p in files.rglob("*") if p.is_file()} if files else {}
        with DiscImage(os.environ["TITBIT_TEST_ISO"]) as iso, DiscImage(os.environ["TITBIT_TEST_RAW"]) as raw:
            self.assertEqual(iso.entries, raw.entries)
            if files:
                # The preserved extraction uses Joliet long names in five
                # entries; this reader intentionally exposes primary ISO names.
                self.assertEqual(len(iso.entries), len(extracted))
            for name, entry in iso.entries.items():
                with self.subTest(name=name), iso.open(name) as a, raw.open(name) as b:
                    iso_hash = digest_file(a)
                    self.assertEqual(iso_hash, digest_file(b))
                    if files:
                        source_path = extracted.get(name)
                        if source_path is None:
                            candidates = [p for key, p in extracted.items()
                                          if key.rpartition("/")[0] == name.rpartition("/")[0]
                                          and p.stat().st_size == entry.size]
                            self.assertEqual(len(candidates), 1, name)
                            source_path = candidates[0]
                        self.assertEqual(source_path.stat().st_size, entry.size)
                        with source_path.open("rb") as source:
                            self.assertEqual(iso_hash, digest_file(source))


if __name__ == "__main__":
    unittest.main()
