"""Generate the project's T icon, without original game artwork or Pillow."""
import struct
from pathlib import Path


def write_icon(path):
    size = 32
    pixels = bytearray()
    for y in range(size-1, -1, -1):
        for x in range(size):
            letter = (7 <= x < 25 and 7 <= y < 12) or (13 <= x < 19 and 10 <= y < 26)
            r, g, b = (24, 36, 39) if letter else (195, 244, 120)
            pixels.extend((b, g, r, 255))
    dib = struct.pack('<IIIHHIIIIII', 40, size, size*2, 1, 32, 0, len(pixels), 0, 0, 0, 0)
    dib += pixels + bytes(((size+31)//32)*4*size)
    data = struct.pack('<HHH', 0, 1, 1) + struct.pack('<BBBBHHII', size, size, 0, 0, 1, 32, len(dib), 22) + dib
    Path(path).write_bytes(data)
