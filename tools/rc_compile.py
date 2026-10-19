"""Compile the game's resource script to a .res file without the Windows SDK.

  python tools/rc_compile.py titbit.rc game.res

Covers the statements used by src/lanes/codex/resources/titbit.rc: LANGUAGE,
ICON, MENU, DIALOG, STRINGTABLE, ACCELERATORS and VERSIONINFO. The output
matches what rc.exe from the Windows SDK writes for this script byte for byte;
anything outside that subset is rejected instead of guessed.
"""
import pathlib
import re
import struct
import sys

PURE, MOVEABLE, DISCARDABLE, PRELOAD = 0x20, 0x10, 0x1000, 0x40
RT_ICON, RT_MENU, RT_DIALOG, RT_STRING, RT_ACCELERATOR, RT_GROUP_ICON, RT_VERSION = 3, 4, 5, 6, 9, 14, 16
CLASSES = {'button': 0x80, 'edit': 0x81, 'static': 0x82, 'listbox': 0x83, 'scrollbar': 0x84, 'combobox': 0x85}
ACCEL = {'VIRTKEY': 1, 'NOINVERT': 2, 'SHIFT': 4, 'CONTROL': 8, 'ALT': 0x10}
ESCAPES = {'n': '\n', 't': '\t', 'r': '\r', 'a': '\b', '\\': '\\', '"': '"', '0': '\0'}
TOKEN = re.compile(r'\s*(?:(//[^\n]*)|("(?:[^"]|"")*")|([^\s,"]+)|(,))')


class RcError(ValueError):
    pass


def tokens(text):
    out, pos = [], 0
    for line_no, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith('#pragma'):
            if not re.fullmatch(r'\s*#pragma\s+code_page\s*\(\s*65001\s*\)\s*', line):
                raise RcError(f'line {line_no}: only #pragma code_page(65001) is supported')
            continue
        pos = 0
        while pos < len(line):
            m = TOKEN.match(line, pos)
            if not m or m.end() == pos:
                if line[pos:].strip():
                    raise RcError(f'line {line_no}: cannot parse {line[pos:]!r}')
                break
            pos = m.end()
            if m.group(1):
                break
            out.append((m.group(2) or m.group(3) or m.group(4), line_no))
    return out


def string_value(token):
    body, out, i = token[1:-1], [], 0
    while i < len(body):
        c = body[i]
        if c == '"':                  # "" inside a string is one quote
            out.append('"'); i += 2; continue
        if c == '\\' and i + 1 < len(body):
            e = body[i + 1]
            if e not in ESCAPES:
                raise RcError('unsupported escape \\' + e)
            out.append(ESCAPES[e]); i += 2; continue
        out.append(c); i += 1
    return ''.join(out)


def wstr(text):
    return text.encode('utf-16le') + b'\0\0'


def pad4(data):
    return data + b'\0' * (-len(data) % 4)


def ident(token):
    return int(token, 0) & 0xFFFF


def res_name(value):
    return struct.pack('<HH', 0xFFFF, value) if isinstance(value, int) else wstr(value.upper())


def res_entry(kind, name, lang, flags, data):
    header = pad4(res_name(kind) + res_name(name))
    header += struct.pack('<IHHII', 0, flags, lang, 0, 0)
    return pad4(struct.pack('<II', len(data), len(header) + 8) + header + data)


class Reader:
    def __init__(self, items):
        self.items, self.i = items, 0

    def peek(self, k=0):
        return self.items[self.i + k][0] if self.i + k < len(self.items) else None

    def next(self):
        if self.i >= len(self.items):
            raise RcError('unexpected end of file')
        self.i += 1
        return self.items[self.i - 1][0]

    def line(self):
        return self.items[min(self.i, len(self.items) - 1)][1]

    def expect(self, word):
        got = self.next()
        if got.upper() != word:
            raise RcError(f'line {self.line()}: expected {word}, got {got}')

    def comma(self):
        if self.peek() == ',':
            self.i += 1

    def numbers(self, count):
        out = []
        for _ in range(count):
            out.append(int(self.next(), 0)); self.comma()
        return out

    def string(self):
        token = self.next()
        if not token.startswith('"'):
            raise RcError(f'line {self.line()}: expected a string, got {token}')
        return string_value(token)


def icon(path, next_id):
    data = path.read_bytes()
    reserved, kind, count = struct.unpack_from('<HHH', data)
    if reserved or kind != 1:
        raise RcError(f'{path}: not an .ico file')
    images, group = [], struct.pack('<HHH', 0, 1, count)
    for n in range(count):
        w, h, colors, _, planes, bits, size, offset = struct.unpack_from('<BBBBHHII', data, 6 + 16 * n)
        image = data[offset:offset + size]
        if image[:4] != b'\x89PNG':   # rc.exe takes planes/bit depth from the DIB header
            planes, bits = struct.unpack_from('<HH', image, 12)
        images.append((next_id + n, image))
        group += struct.pack('<BBBBHHIH', w, h, colors, 0, planes, bits, size, next_id + n)
    return images, group


def menu(r):
    def items():
        r.expect('BEGIN')
        parsed = []
        while r.peek().upper() != 'END':
            kind = r.next().upper()
            if kind == 'POPUP':
                parsed.append(('popup', r.string(), items()))
            elif kind == 'MENUITEM' and r.peek().upper() == 'SEPARATOR':
                r.next(); parsed.append(('item', '', 0))
            elif kind == 'MENUITEM':
                text = r.string(); r.comma()
                parsed.append(('item', text, ident(r.next())))
            else:
                raise RcError(f'line {r.line()}: unsupported menu statement {kind}')
        r.next()
        return parsed

    def emit(entries):
        out = b''
        for n, (kind, text, value) in enumerate(entries):
            end = 0x80 if n == len(entries) - 1 else 0
            if kind == 'popup':
                out += struct.pack('<H', 0x10 | end) + wstr(text) + emit(value)
            else:
                out += struct.pack('<HH', end, value) + wstr(text)
        return out

    return struct.pack('<HH', 0, 0) + emit(items())


def dialog(r):
    x, y, cx, cy = r.numbers(4)
    style, exstyle, caption, font = 0x80880000, 0, None, None
    while r.peek().upper() != 'BEGIN':
        word = r.next().upper()
        if word == 'STYLE':
            style = int(r.next(), 0)
        elif word == 'EXSTYLE':
            exstyle = int(r.next(), 0)
        elif word == 'CAPTION':
            caption = r.string()
        elif word == 'FONT':
            size = int(r.next(), 0); r.comma(); font = (size, r.string())
        else:
            raise RcError(f'line {r.line()}: unsupported dialog option {word}')
    if font:
        style |= 0x40               # DS_SETFONT
    if caption is not None:
        style |= 0x00C00000         # WS_CAPTION
    r.next()
    controls = []
    while r.peek().upper() != 'END':
        r.expect('CONTROL')
        token = r.next()
        text = string_value(token) if token.startswith('"') else int(token, 0)
        r.comma(); cid = ident(r.next()); r.comma()
        cls = r.string().lower(); r.comma()
        cstyle, cx_, cy_, cw, ch = r.numbers(5)
        cex = int(r.next(), 0) if r.peek() not in (None, 'END', 'CONTROL') else 0
        if cls not in CLASSES:
            raise RcError(f'line {r.line()}: unsupported control class {cls}')
        controls.append((cstyle | 0x50000000, cex, cx_, cy_, cw, ch, cid, cls, text))
    r.next()
    out = struct.pack('<IIHhhhh', style, exstyle, len(controls), x, y, cx, cy)
    out += b'\0\0' + b'\0\0' + wstr(caption or '')
    if font:
        out += struct.pack('<H', font[0]) + wstr(font[1])
    for cstyle, cex, cx_, cy_, cw, ch, cid, cls, text in controls:
        out = pad4(out)
        out += struct.pack('<IIhhhhH', cstyle, cex, cx_, cy_, cw, ch, cid)
        out += struct.pack('<HH', 0xFFFF, CLASSES[cls])
        out += struct.pack('<HH', 0xFFFF, text) if isinstance(text, int) else wstr(text)
        out += b'\0\0'
    return out


def accelerators(r):
    r.expect('BEGIN')
    rows = []
    while r.peek().upper() != 'END':
        key = r.next(); r.comma(); cid = ident(r.next())
        flags = 0
        while r.peek() == ',':
            r.next(); word = r.next().upper()
            if word not in ACCEL:
                raise RcError(f'line {r.line()}: unsupported accelerator flag {word}')
            flags |= ACCEL[word]
        if not flags & 1:
            raise RcError(f'line {r.line()}: only VIRTKEY accelerators are supported')
        rows.append([flags, int(key, 0), cid])
    r.next()
    rows[-1][0] |= 0x80
    return b''.join(struct.pack('<HHHH', f, k, c, 0) for f, k, c in rows)


def version_node(key, value=b'', value_len=0, text=False, children=b''):
    body = pad4(struct.pack('<HHH', 0, value_len, 1 if text else 0) + wstr(key)) + value
    if children:
        body = pad4(body) + children
    return struct.pack('<H', len(body)) + body[2:]


def version_block(r):
    r.expect('BEGIN')
    out = b''
    while r.peek().upper() != 'END':
        word = r.next().upper()
        if word == 'BLOCK':
            key = r.string()
            out = pad4(out) + version_node(key, text=True, children=version_block(r))
        elif word == 'VALUE':
            key = r.string(); r.comma()
            if r.peek().startswith('"'):
                text = r.string()
                value = wstr(text) if text else b''
                out = pad4(out) + version_node(key, value, len(text) + 1 if text else 0, True)
            else:
                words = []
                while True:
                    words.append(int(r.next(), 0))
                    if r.peek() != ',':
                        break
                    r.next()
                out = pad4(out) + version_node(key, struct.pack(f'<{len(words)}H', *words), 2 * len(words))
        else:
            raise RcError(f'line {r.line()}: unsupported VERSIONINFO statement {word}')
    r.next()
    return out


def versioninfo(r):
    fixed = {'FILEVERSION': [0] * 4, 'PRODUCTVERSION': [0] * 4}
    while r.peek().upper() != 'BEGIN':
        word = r.next().upper()
        if word in ('FILEVERSION', 'PRODUCTVERSION'):
            fixed[word] = r.numbers(4)
        elif word in ('FILEFLAGSMASK', 'FILEFLAGS', 'FILEOS', 'FILETYPE', 'FILESUBTYPE'):
            fixed[word] = int(r.next(), 0)
        else:
            raise RcError(f'line {r.line()}: unsupported VERSIONINFO option {word}')
    fv, pv = fixed['FILEVERSION'], fixed['PRODUCTVERSION']
    info = struct.pack('<13I', 0xFEEF04BD, 0x10000, fv[0] << 16 | fv[1], fv[2] << 16 | fv[3],
                       pv[0] << 16 | pv[1], pv[2] << 16 | pv[3], fixed.get('FILEFLAGSMASK', 0),
                       fixed.get('FILEFLAGS', 0), fixed.get('FILEOS', 0), fixed.get('FILETYPE', 0),
                       fixed.get('FILESUBTYPE', 0), 0, 0)
    return version_node('VS_VERSION_INFO', info, len(info), children=version_block(r))


def compile_rc(source):
    source = pathlib.Path(source)
    r = Reader(tokens(source.read_text(encoding='utf-8-sig')))
    lang, icon_id = 0x0409, 1
    out = [res_entry(0, 0, 0, 0, b'')]
    strings = {}
    while r.peek() is not None:
        word = r.next()
        if word.upper() == 'LANGUAGE':
            primary, sub = r.numbers(2)
            lang = sub << 10 | primary
            continue
        if word.upper() == 'STRINGTABLE':
            r.expect('BEGIN')
            while r.peek().upper() != 'END':
                sid = ident(r.next()); r.comma()
                block = strings.setdefault((sid // 16 + 1, lang), [''] * 16)
                block[sid % 16] = r.string()
            r.next()
            continue
        name = ident(word) if re.fullmatch(r'-?(0x[0-9a-fA-F]+|\d+)', word) else word
        kind = r.next().upper()
        if kind == 'ICON':
            images, group = icon(source.parent / r.string(), icon_id)
            icon_id += len(images)
            out += [res_entry(RT_ICON, n, lang, MOVEABLE | DISCARDABLE, data) for n, data in images]
            out.append(res_entry(RT_GROUP_ICON, name, lang, MOVEABLE | PURE | DISCARDABLE, group))
        elif kind == 'MENU':
            out.append(res_entry(RT_MENU, name, lang, MOVEABLE | PURE | DISCARDABLE, menu(r)))
        elif kind == 'DIALOG':
            out.append(res_entry(RT_DIALOG, name, lang, MOVEABLE | PURE | DISCARDABLE, dialog(r)))
        elif kind == 'ACCELERATORS':
            out.append(res_entry(RT_ACCELERATOR, name, lang, MOVEABLE | PURE, accelerators(r)))
        elif kind == 'VERSIONINFO':
            out.append(res_entry(RT_VERSION, name, lang, MOVEABLE | PURE, versioninfo(r)))
        else:
            raise RcError(f'line {r.line()}: unsupported resource type {kind}')
    for (block, block_lang), texts in sorted(strings.items()):
        data = b''.join(struct.pack('<H', len(t.encode('utf-16le')) // 2) + t.encode('utf-16le') for t in texts)
        out.append(res_entry(RT_STRING, block, block_lang, MOVEABLE | PURE | DISCARDABLE, data))
    return b''.join(out)


def main(argv):
    if len(argv) != 3:
        print(__doc__.strip().splitlines()[2].strip())
        return 2
    try:
        pathlib.Path(argv[2]).write_bytes(compile_rc(argv[1]))
    except (RcError, OSError, struct.error) as error:
        print('rc_compile:', error, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
