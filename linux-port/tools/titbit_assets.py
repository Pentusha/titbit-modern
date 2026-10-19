"""Read original Titbit data from a directory, ISO or InstallShield 6 cabinet.

Standard library only. No installer execution, mounting, downloads or registry
changes. Import is staged and hash-checked before existing files are touched.
InstallShield layout facts: twogood/unshield lib/cabfile.h and lib/file.c;
the bounded implementation here is specific to the original Titbit disc.
"""
from __future__ import annotations

from contextlib import contextmanager
from dataclasses import dataclass, field
import hashlib
import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import struct
import sys
import tempfile
import zlib

from disc_image import DiscImage

ROOT = Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parents[1]))
MANIFEST = ROOT / 'config' / 'assets.json'
BLOCK = 1024 * 1024


class ImportCancelled(ValueError):
    pass


def check_cancel(cancel):
    if cancel is not None and cancel.is_set():
        raise ImportCancelled('Импорт отменён. Можно повторить его позже.')


def manifest():
    try:
        data = json.loads(MANIFEST.read_text(encoding='utf-8'))
        files = data['files']
        if data.get('version') != 1 or not isinstance(files, dict) or len(files) != 19:
            raise ValueError('version/count')
        for name, record in files.items():
            if (not isinstance(name, str) or Path(name).name != name or '\\' in name or '/' in name
                    or type(record['size']) is not int or not 0 < record['size'] < 2**32
                    or not isinstance(record['sha256'], str)
                    or not re.fullmatch('[0-9a-f]{64}', record['sha256'])):
                raise ValueError('record')
        if len({n.lower() for n in files}) != 19 or 'QUEST.ezd' not in files:
            raise ValueError('names')
        return files
    except (OSError, UnicodeError, ValueError, KeyError, TypeError, AttributeError) as exc:
        raise ValueError('Повреждён список файлов игры. Распакуйте пакет заново.') from exc


@dataclass
class Asset:
    name: str
    size: int
    sha256: str

    def chunks(self):
        yield from self._chunks()


@dataclass
class Plan:
    origin: str
    kind: str
    description: str
    files: dict[str, Asset]
    warnings: list[str] = field(default_factory=list)

    def close(self):
        reader = getattr(self, '_reader', None)
        if reader is not None:
            reader.close()
            self._reader = None

    def __del__(self):
        self.close()


def read_exact(stream, count):
    data = stream.read(count)
    if len(data) != count:
        raise ValueError('Источник повреждён или не полностью скопирован.')
    return data


def file_chunks(path):
    with path.open('rb') as stream:
        while data := stream.read(BLOCK):
            yield data


class Directory:
    def __init__(self, path):
        self.path = path.resolve()
        self.entries = {}
        directories = 0
        for folder, dirs, files in os.walk(self.path, followlinks=False):
            folder = Path(folder)
            depth = len(folder.relative_to(self.path).parts)
            dirs[:] = [d for d in dirs if not (folder/d).is_symlink()] if depth < 5 else []
            directories += 1
            if directories > 2000:
                raise ValueError('Выберите папку самой игры или корень диска, а не весь накопитель.')
            for name in files:
                p = folder/name
                if p.is_symlink():
                    continue
                key = p.relative_to(self.path).as_posix().lower()
                if key in self.entries:
                    raise ValueError('В источнике есть неоднозначные имена файлов.')
                self.entries[key] = p

    def size(self, name):
        return self.entries[name.lower()].stat().st_size

    def open(self, name):
        return self.entries[name.lower()].open('rb')

    def close(self):
        pass


class Image:
    def __init__(self, path):
        self.reader = DiscImage(path)
        self.entries = self.reader.entries

    def size(self, name):
        return self.entries[name.lower()].size

    def open(self, name):
        return self.reader.open(name)

    def close(self):
        self.reader.close()


class Cabinet:
    """InstallShield 6 fixed-size file descriptors, independent deflate chunks.

    Deliberately reject split, linked and obfuscated entries: none is used for
    the nineteen game assets on this disc. Bounds and both MD5/SHA256 are checked.
    """
    def __init__(self, source, header):
        self.source = source
        self.folder = PurePosixPath(header).parent
        if source.size(header) > 16*1024*1024:
            raise ValueError('Слишком большой заголовок InstallShield.')
        with source.open(header) as stream:
            data = read_exact(stream, source.size(header))
        if len(data) < 20:
            raise ValueError('Обрезанный заголовок InstallShield.')
        signature, version, _, desc, desc_size = struct.unpack_from('<5I', data)
        if signature != 0x28635349 or ((version >> 12) & 0xff) != 6:
            raise ValueError('Эта версия InstallShield не поддерживается. Выберите установленную игру.')
        if desc > len(data)-0x30 or desc_size > len(data)-desc:
            raise ValueError('Повреждённое описание InstallShield.')
        table = desc + struct.unpack_from('<I', data, desc+0xc)[0]
        count = struct.unpack_from('<I', data, desc+0x28)[0]
        descriptors = table + struct.unpack_from('<I', data, desc+0x2c)[0]
        if count > 100000 or descriptors > len(data) or descriptors + count*87 > len(data):
            raise ValueError('Повреждена таблица файлов InstallShield.')
        wanted = {name.lower(): name for name in manifest()}
        self.files = {}
        for i in range(count):
            p = descriptors + i*87
            flags, size, packed, offset = struct.unpack_from('<HQQQ', data, p)
            name_offset = table + struct.unpack_from('<I', data, p+58)[0]
            if flags & 8 or name_offset >= len(data):
                continue
            end = data.find(b'\0', name_offset)
            if end < 0 or end-name_offset > 1024:
                raise ValueError('Повреждено имя файла InstallShield.')
            name = data[name_offset:end].decode('cp1251').lower()
            if name not in wanted:
                continue
            if name in self.files or flags & 3 or data[p+84] or flags & ~12:
                raise ValueError('Неподдерживаемая запись InstallShield: '+name)
            volume = struct.unpack_from('<H', data, p+85)[0]
            member = (self.folder/f'data{volume}.cab').as_posix().lower()
            if member not in source.entries or offset < 84 or packed > source.size(member)-offset:
                raise ValueError('Не хватает части установочного диска: '+member)
            self.files[name] = (member, flags, size, packed, offset, data[p+26:p+42])

    def chunks(self, name):
        member, flags, expanded, packed, offset, expected_md5 = self.files[name.lower()]
        produced = 0
        checksum = hashlib.md5()
        with self.source.open(member) as stream:
            common = read_exact(stream, 20)
            if struct.unpack_from('<I', common)[0] != 0x28635349:
                raise ValueError('Повреждён кабинет InstallShield.')
            stream.seek(offset)
            remaining = packed
            while remaining:
                if flags & 4:
                    if remaining < 2:
                        raise ValueError('Обрезан блок InstallShield.')
                    count = struct.unpack('<H', read_exact(stream, 2))[0]
                    remaining -= 2
                    if not 0 < count <= remaining:
                        raise ValueError('Повреждён размер блока InstallShield.')
                    raw = read_exact(stream, count)
                    remaining -= count
                    decoder = zlib.decompressobj(-15)
                    try:
                        chunk = decoder.decompress(raw+b'\0', 65537)
                    except zlib.error as exc:
                        raise ValueError('Ошибка распаковки: '+name) from exc
                    if len(chunk) > 65536 or not decoder.eof or decoder.unconsumed_tail:
                        raise ValueError('Неполный или слишком большой блок: '+name)
                else:
                    count = min(remaining, BLOCK)
                    chunk = read_exact(stream, count)
                    remaining -= count
                produced += len(chunk)
                if produced > expanded:
                    raise ValueError('Размер распакованных данных не совпал: '+name)
                checksum.update(chunk)
                yield chunk
        if produced != expanded or checksum.digest() != expected_md5:
            raise ValueError('Контрольная сумма InstallShield не совпала: '+name)


def inspect_source(path):
    path = Path(path).expanduser().resolve()
    if not path.exists():
        raise ValueError('Источник не найден. Выберите папку игры или образ диска.')
    if path.is_file() and path.suffix.lower() in ('.ezd', '.cab', '.hdr'):
        path = path.parent
    source = Directory(path) if path.is_dir() else Image(path)
    expected = manifest()
    try:
        # Prefer an already installed game. Every file must be from one directory.
        parents = sorted({str(PurePosixPath(k).parent) for k in source.entries
                          if PurePosixPath(k).name.lower() == 'quest.ezd'})
        candidates = []
        for parent in parents:
            names = {name:(PurePosixPath(parent)/name).as_posix().lower() for name in expected}
            if all(member in source.entries for member in names.values()):
                candidates.append(names)
        files = {}
        if len(candidates) > 1:
            raise ValueError('Найдено несколько копий игры. Выберите папку одной копии.')
        if candidates:
            for name, member in candidates[0].items():
                record = expected[name]
                if source.size(member) != record['size']:
                    raise ValueError('Размер '+name+' отличается от версии 1.00. Выберите другую копию.')
                asset = Asset(name, record['size'], record['sha256'])
                def chunks(member=member):
                    with source.open(member) as stream:
                        while data := stream.read(BLOCK):
                            yield data
                asset._chunks = chunks
                files[name] = asset
            kind, description = 'folder', 'Готовые файлы игры'
        else:
            headers = [k for k in source.entries if PurePosixPath(k).name.lower() == 'data1.hdr']
            if len(headers) != 1:
                raise ValueError('Не найдены QUEST.ezd и все 18 AVI либо установочные data1.hdr/data2.cab. '
                                 'Выберите папку установленной игры или оригинальный диск/образ.')
            cab = Cabinet(source, headers[0])
            for name, record in expected.items():
                entry = cab.files.get(name.lower())
                if entry is None or entry[2] != record['size']:
                    raise ValueError('В установочных файлах нет подходящего '+name+'.')
                asset = Asset(name, record['size'], record['sha256'])
                asset._chunks = lambda name=name: cab.chunks(name)
                files[name] = asset
            kind, description = 'installshield6', 'Оригинальный установочный диск'
        plan = Plan(str(path), kind, description, files)
        plan._reader = source
        return plan
    except BaseException:
        source.close()
        raise


def hash_file(path, progress=None, done=0, total=0, cancel=None):
    checksum = hashlib.sha256()
    for block in file_chunks(path):
        check_cancel(cancel)
        checksum.update(block)
        done += len(block)
        if progress:
            progress(path.name, done, total)
    return checksum.hexdigest()


def verify_install(destination, progress=None, cancel=None):
    target = Path(destination).expanduser().resolve()
    expected = manifest()
    total = sum(v['size'] for v in expected.values())
    done = 0
    verified = []
    for name, record in expected.items():
        check_cancel(cancel)
        path = target/name
        if path.is_symlink() or not path.is_file() or path.stat().st_size != record['size']:
            raise ValueError('Файл отсутствует или имеет неверный размер: '+name)
        actual = hash_file(path, progress, done, total, cancel)
        if actual != record['sha256']:
            raise ValueError('Файл повреждён или изменён: '+name)
        done += record['size']
        verified.append(name)
    return {'verified': len(verified), 'bytes': done, 'directory': str(target)}


def install_assets(plan, destination, progress=None, cancel=None):
    target = Path(destination).expanduser().absolute()
    if target.is_symlink():
        raise ValueError('Папка назначения не должна быть ссылкой.')
    target = target.resolve()
    expected = manifest()
    if set(plan.files) != set(expected):
        raise ValueError('Неполный список файлов игры.')
    target.mkdir(parents=True, exist_ok=True)
    # Respect modified archives, junctions, saves and all unrelated user files.
    reused, missing = [], []
    for name, record in expected.items():
        check_cancel(cancel)
        dest = target/name
        if dest.is_symlink():
            raise ValueError('Файл назначения является ссылкой: '+name)
        if dest.exists():
            if not dest.is_file() or dest.stat().st_size != record['size'] or hash_file(dest, cancel=cancel) != record['sha256']:
                raise ValueError('В папке уже есть другой '+name+'. Выберите отдельную пустую папку; '
                                 'изменённые данные не перезаписываются.')
            reused.append(name)
        else:
            missing.append(name)
    required = sum(expected[n]['size'] for n in missing)
    if shutil.disk_usage(target).free < required + 16*1024*1024:
        raise ValueError('Недостаточно свободного места в папке назначения.')
    total = sum(v['size'] for v in expected.values())
    done = sum(expected[n]['size'] for n in reused)
    if progress:
        progress('Проверены существующие файлы', done, total)
    # Stage all data first. Failed/cancelled extraction never commits partial assets.
    with tempfile.TemporaryDirectory(prefix='.titbit-import-', dir=target) as work:
        stage = Path(work)
        for name in missing:
            check_cancel(cancel)
            asset = plan.files[name]
            checksum, size = hashlib.sha256(), 0
            with (stage/name).open('xb') as output:
                for block in asset.chunks():
                    check_cancel(cancel)
                    size += len(block)
                    if size > expected[name]['size']:
                        raise ValueError('Слишком большой файл: '+name)
                    output.write(block)
                    checksum.update(block)
                    done += len(block)
                    if progress:
                        progress(name, done, total)
            if size != expected[name]['size'] or checksum.hexdigest() != expected[name]['sha256']:
                raise ValueError('Проверка SHA256 не пройдена: '+name+'. Данные не установлены.')
        check_cancel(cancel)
        created = []
        try:
            for name in missing:
                # Hard-link creation is atomic and fails if another process created
                # the destination meanwhile. It never replaces a user file.
                identity = (stage/name).stat()
                if os.name == 'nt':
                    # Windows rename is atomic and refuses an existing target;
                    # unlike hardlinks, it also works on FAT/exFAT USB drives.
                    os.rename(stage/name, target/name)
                else:
                    os.link(stage/name, target/name)
                created.append((name, identity.st_dev, identity.st_ino))
        except BaseException:
            for name, device, inode in created:
                try:
                    current = (target/name).stat()
                    if (current.st_dev, current.st_ino) == (device, inode):
                        (target/name).unlink()
                except OSError:
                    pass
            raise
    return {'installed': len(missing), 'reused': len(reused), 'verified': 19,
            'bytes': total, 'directory': str(target)}
