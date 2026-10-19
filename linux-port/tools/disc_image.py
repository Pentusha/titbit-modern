"""Read-only ISO 9660 files from ISO or single-track MODE1/2352 images.

No mounting, administrator rights, external programs or third-party modules.
Only the primary ISO 9660 tree is used; image contents are never executed.
"""
from __future__ import annotations

import configparser
import io
from pathlib import Path
import re
import threading
from dataclasses import dataclass


class DiscImageError(ValueError):
    """Unsupported or malformed disc image."""


@dataclass(frozen=True)
class DiscEntry:
    name: str
    size: int
    extent: int


def _both(data: bytes, offset: int, width: int) -> int:
    first = int.from_bytes(data[offset:offset + width], "little")
    second = int.from_bytes(data[offset + width:offset + width * 2], "big")
    if first != second:
        raise DiscImageError("Inconsistent ISO 9660 byte order fields")
    return first


def _safe_name(value: str) -> str:
    if (not value or value in (".", "..") or
            any(c in value for c in '/\\:') or
            any(ord(c) < 32 or ord(c) == 127 for c in value)):
        raise DiscImageError("Unsafe ISO 9660 file name")
    return value


def _image_reference(path: Path) -> tuple[Path, str | None]:
    suffix = path.suffix.lower()
    if suffix == ".cue":
        try:
            text = path.read_text(encoding="utf-8-sig")
        except UnicodeError as exc:
            raise DiscImageError("CUE must use UTF-8 or ASCII") from exc
        files, tracks, indices = [], [], []
        for line in text.splitlines():
            line = line.strip()
            if not line or line.upper().startswith("REM "):
                continue
            if match := re.fullmatch(r'FILE\s+"([^"\r\n]+)"\s+BINARY', line, re.I):
                files.append(match.group(1))
            elif match := re.fullmatch(r'TRACK\s+(\d+)\s+(\S+)', line, re.I):
                tracks.append((int(match.group(1)), match.group(2).upper()))
            elif match := re.fullmatch(r'INDEX\s+(\d+)\s+(\d+):(\d+):(\d+)', line, re.I):
                indices.append(tuple(map(int, match.groups())))
            else:
                raise DiscImageError("Unsupported CUE directive; use a single MODE1/2352 track")
        if len(files) != 1 or tracks != [(1, "MODE1/2352")] or indices != [(1, 0, 0, 0)]:
            raise DiscImageError("CUE must contain one MODE1/2352 track starting at 00:00:00")
        # Do not allow a descriptor to point outside its own directory.
        name = _safe_name(files[0])
        target = (path.parent / name).resolve()
        if target.parent != path.parent.resolve():
            raise DiscImageError("CUE image reference escapes its directory")
        return target, "raw"
    if suffix == ".ccd":
        config = configparser.ConfigParser(interpolation=None, strict=True)
        try:
            config.read_string(path.read_text(encoding="utf-8-sig"))
            tracks = [s for s in config.sections() if s.upper().startswith("TRACK ")]
            if (tracks != ["TRACK 1"] or config.getint("TRACK 1", "MODE") != 1 or
                    config.getint("TRACK 1", "INDEX 1") != 0 or
                    config.getint("Disc", "Sessions") != 1 or
                    config.getint("Disc", "DataTracksScrambled", fallback=0) != 0):
                raise DiscImageError("CCD must describe one unscrambled MODE1 track at sector zero")
        except (configparser.Error, UnicodeError, ValueError) as exc:
            raise DiscImageError("Unsupported CCD descriptor: " + str(exc)) from exc
        target = path.with_suffix(".img")
        if not target.exists():
            candidates = [p for p in path.parent.iterdir()
                          if p.name.lower() == target.name.lower()]
            if len(candidates) != 1:
                raise DiscImageError("CCD companion IMG file was not found")
            target = candidates[0]
        if target.resolve().parent != path.parent.resolve():
            raise DiscImageError("CCD image reference escapes its directory")
        return target, "raw"
    return path, None


class _DiscFile(io.RawIOBase):
    def __init__(self, image: "DiscImage", entry: DiscEntry):
        super().__init__()
        self._image, self._entry, self._position = image, entry, 0

    def readable(self):
        return True

    def seekable(self):
        return True

    def tell(self):
        self._checkClosed()
        return self._position

    def seek(self, offset, whence=io.SEEK_SET):
        self._checkClosed()
        if whence == io.SEEK_SET:
            position = offset
        elif whence == io.SEEK_CUR:
            position = self._position + offset
        elif whence == io.SEEK_END:
            position = self._entry.size + offset
        else:
            raise ValueError("Invalid seek origin")
        if position < 0 or position > self._entry.size:
            raise ValueError("Seek outside disc file")
        self._position = position
        return position

    def read(self, size=-1):
        self._checkClosed()
        remaining = self._entry.size - self._position
        size = remaining if size is None or size < 0 else min(size, remaining)
        data = self._image._read(self._entry.extent * 2048 + self._position, size)
        self._position += len(data)
        return data

    def readinto(self, buffer):
        data = self.read(len(buffer))
        buffer[:len(data)] = data
        return len(data)


class DiscImage:
    """ISO reader. ``entries`` uses lowercase POSIX paths; names retain ISO case.

    ``open(name)`` returns an independent, bounded binary file view. All source
    files are opened read-only. CUE/CCD support deliberately excludes audio,
    multisession, scrambled and multi-track discs.
    """
    def __init__(self, path):
        self.path = Path(path)
        target, kind = _image_reference(self.path)
        self.image_path = target
        self._file = target.open("rb")
        self._lock = threading.RLock()
        self.entries: dict[str, DiscEntry] = {}
        self.directories: set[str] = set()
        try:
            self._physical_size = self._file.seek(0, io.SEEK_END)
            self.sector_size, self._payload = self._detect(kind)
            if self._physical_size % self.sector_size:
                raise DiscImageError("Truncated disc sector")
            self._logical_size = self._physical_size // self.sector_size * 2048
            primary = None
            for sector in range(16, min(16 + 128, self._logical_size // 2048)):
                record = self._read(sector * 2048, 2048)
                if record[1:6] != b"CD001" or record[6] != 1:
                    raise DiscImageError("Invalid ISO 9660 volume descriptor")
                if record[0] == 1:
                    if primary is not None:
                        raise DiscImageError("Multiple primary ISO volume descriptors")
                    primary = record
                if record[0] == 255:
                    break
            else:
                raise DiscImageError("ISO volume descriptor terminator was not found")
            if primary is None:
                raise DiscImageError("ISO primary volume descriptor was not found")
            if _both(primary, 128, 2) != 2048:
                raise DiscImageError("Only 2048-byte ISO logical blocks are supported")
            volume_size = _both(primary, 80, 4) * 2048
            if not volume_size or volume_size > self._logical_size:
                raise DiscImageError("ISO volume extends beyond the image")
            self._logical_size = volume_size
            root = self._record(primary[156:156 + primary[156]])
            if not root[2] & 2 or root[3] != b"\0":
                raise DiscImageError("Invalid ISO root directory")
            self._scan(root[0], root[1], "", set(), 0, set())
        except BaseException:
            self.close()
            raise

    def _detect(self, kind):
        self._file.seek(16 * 2048)
        iso = self._file.read(7)
        self._file.seek(16 * 2352)
        raw = self._file.read(23)
        if kind != "raw" and len(iso) == 7 and iso[1:6] == b"CD001" and iso[6] == 1:
            return 2048, 0
        if (len(raw) == 23 and raw[:12] == b"\0" + b"\xff" * 10 + b"\0" and
                raw[15] == 1 and raw[17:22] == b"CD001" and raw[22] == 1):
            return 2352, 16
        raise DiscImageError("Expected an ISO 9660 ISO or MODE1/2352 IMG image")

    def _read(self, offset, size):
        if offset < 0 or size < 0 or offset + size > self._logical_size:
            raise DiscImageError("Read outside ISO volume")
        with self._lock:
            if self._file.closed:
                raise ValueError("Disc image is closed")
            if self.sector_size == 2048:
                self._file.seek(offset)
                result = self._file.read(size)
            else:
                chunks = []
                while size:
                    sector, within = divmod(offset, 2048)
                    count = min(size, 512 * 2048 - within)
                    sectors = (within + count + 2047) // 2048
                    self._file.seek(sector * 2352)
                    data = self._file.read(sectors * 2352)
                    if len(data) != sectors * 2352:
                        raise DiscImageError("Truncated MODE1 sector")
                    payloads = []
                    for start in range(0, len(data), 2352):
                        if (data[start:start + 12] != b"\0" + b"\xff" * 10 + b"\0" or
                                data[start + 15] != 1):
                            raise DiscImageError("Invalid MODE1 sector")
                        payloads.append(data[start + 16:start + 2064])
                    chunks.append(b"".join(payloads)[within:within + count])
                    offset += count
                    size -= count
                result = b"".join(chunks)
            if len(result) != size and self.sector_size == 2048:
                raise DiscImageError("Truncated ISO file data")
            return result

    def _record(self, record):
        if len(record) < 34 or record[0] != len(record) or 33 + record[32] > len(record):
            raise DiscImageError("Invalid ISO directory record")
        if record[26] or record[27]:
            raise DiscImageError("Interleaved ISO files are unsupported")
        if record[25] & 0x80:
            raise DiscImageError("Multi-extent ISO files are unsupported")
        extent = _both(record, 2, 4) + record[1]
        size = _both(record, 10, 4)
        if _both(record, 28, 2) != 1:
            raise DiscImageError("Multi-volume ISO files are unsupported")
        if extent * 2048 + size > self._logical_size:
            raise DiscImageError("ISO file extent exceeds volume bounds")
        return extent, size, record[25], record[33:33 + record[32]]

    def _scan(self, extent, size, prefix, visited, depth, names):
        if depth > 64 or extent in visited or size > 64 * 1024 * 1024:
            raise DiscImageError("Cyclic or excessive ISO directory tree")
        visited.add(extent)
        data = self._read(extent * 2048, size)
        offset = 0
        while offset < len(data):
            length = data[offset]
            if not length:
                end = min(((offset // 2048) + 1) * 2048, len(data))
                if any(data[offset:end]):
                    raise DiscImageError("Invalid ISO directory padding")
                offset = end
                continue
            if offset % 2048 + length > 2048 or offset + length > len(data):
                raise DiscImageError("ISO directory record crosses block boundary")
            child_extent, child_size, flags, raw_name = self._record(data[offset:offset + length])
            offset += length
            if raw_name in (b"\0", b"\1"):
                continue
            try:
                name = _safe_name(re.sub(r";\d+$", "", raw_name.decode("ascii")))
            except UnicodeError as exc:
                raise DiscImageError("Non-ASCII ISO primary file name") from exc
            name = prefix + name
            key = name.lower()
            if key in names or len(names) >= 100000:
                raise DiscImageError("Duplicate, case-colliding or excessive ISO paths")
            names.add(key)
            if flags & 2:
                self.directories.add(key)
                self._scan(child_extent, child_size, name + "/", visited, depth + 1, names)
            else:
                self.entries[key] = DiscEntry(name, child_size, child_extent)

    def open(self, name):
        if self._file.closed:
            raise ValueError("Disc image is closed")
        if not isinstance(name, str):
            raise TypeError("Disc path must be a string")
        for component in name.split("/"):
            _safe_name(component)
        try:
            entry = self.entries[name.lower()]
        except KeyError:
            raise FileNotFoundError(name) from None
        return _DiscFile(self, entry)

    def close(self):
        self._file.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
