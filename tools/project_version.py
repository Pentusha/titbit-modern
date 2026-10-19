"""One version number and one set of file properties for every EXE we ship.

The properties name this project, not the original developer: the files are an
unofficial source port, signed by nobody, and must not look like a VZlab release.
"""
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]
VERSION = (BASE / 'VERSION').read_text(encoding='ascii').strip() if (BASE / 'VERSION').is_file() else '0.0.0'
COMPANY = 'titbit-modern (unofficial source port)'
PRODUCT = 'Nuclear Titbit - titbit-modern'
COPYRIGHT = 'Game (c) 2003 VZlab / Buka. Not affiliated with the rights holders.'
COMMENTS = 'Source: https://github.com/offx366/titbit-modern'


def numbers():
    parts = [int(p) for p in VERSION.split('-')[0].split('.')]
    return tuple((parts + [0, 0, 0, 0])[:4])


def strings(filename, description):
    return [('CompanyName', COMPANY), ('FileDescription', description), ('FileVersion', VERSION),
            ('InternalName', Path(filename).stem), ('LegalCopyright', COPYRIGHT),
            ('OriginalFilename', filename), ('ProductName', PRODUCT), ('ProductVersion', VERSION),
            ('Comments', COMMENTS)]


def versioninfo_rc(filename, description, filetype=1):
    """VERSIONINFO statement for tools/rc_compile.py (filetype 1 = application)."""
    n = ', '.join(map(str, numbers()))
    values = '\n'.join(f'   VALUE "{k}", "{v}"' for k, v in strings(filename, description))
    return (f'LANGUAGE 0x9, 0x1\n\n1 VERSIONINFO\nFILEVERSION {n}\nPRODUCTVERSION {n}\n'
            f'FILEFLAGSMASK 0x3f\nFILEFLAGS 0x0\nFILEOS 0x4\nFILETYPE {filetype:#x}\nFILESUBTYPE 0x0\n'
            'BEGIN\n BLOCK "StringFileInfo"\n BEGIN\n  BLOCK "040904b0"\n  BEGIN\n'
            f'{values}\n  END\n END\n BLOCK "VarFileInfo"\n BEGIN\n  VALUE "Translation", 0x409, 0x4b0\n END\nEND\n')


def pyinstaller_version_file(path, filename, description):
    """Version resource in PyInstaller's --version-file format."""
    n = numbers()
    table = ',\n'.join(f'        StringStruct({k!r}, {v!r})' for k, v in strings(filename, description))
    Path(path).write_text(
        'VSVersionInfo(\n'
        f'  ffi=FixedFileInfo(filevers={n}, prodvers={n}, mask=0x3f, flags=0x0, OS=0x40004,\n'
        '                    fileType=0x1, subtype=0x0, date=(0, 0)),\n'
        '  kids=[\n'
        "    StringFileInfo([StringTable('040904B0', [\n"
        f'{table}])]),\n'
        "    VarFileInfo([VarStruct('Translation', [1033, 1200])])])\n", encoding='utf-8')
