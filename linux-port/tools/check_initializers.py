#!/usr/bin/env python3
"""Check real source-owned global state at native game-loop entry, without a window or assets."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GDB_CHECK = r'''
set pagination off
set confirm off
break titbit_linux_run
run
python
import gdb, json, struct
inferior = gdb.selected_inferior()
def memory(symbol, offset, count):
    address = int(gdb.parse_and_eval('(void*)&' + symbol))
    return bytes(inferior.read_memory(address + offset, count))
def words(symbol, count):
    return struct.unpack('<' + 'I'*count, memory(symbol, 0, count*4))
checks = []
for symbol in ('walk_area_cache', 'mesh_record_cache'):
    assert words(symbol, 3) == (5, 0, 0), symbol
    checks.append(symbol)
resource = words('resource_cache_storage', 5)
assert resource[0] and resource[1:] == (0, 0, 0, 10), resource
checks.append('resource cache virtual methods and growth')
texture = words('texture_cache_storage', 3)
assert texture[0] and texture[1:] == (0, 0), texture
checks.append('texture cache virtual methods')
assert struct.unpack('<i', memory('frame_renderer_storage', 16, 4))[0] == 0x4000
assert struct.unpack('<ff', memory('frame_renderer_storage', 0xec004, 8)) == (1., 40.)
checks.append('renderer light index / zoom / FOV')
assert struct.unpack('<16f', memory('scene_identity_matrix', 0, 64)) == tuple(float(i%5 == 0) for i in range(16))
checks.append('identity matrix')
sound = words('sound_bank_storage', 4)
assert sound[0] and sound[1:] == (5, 0, 0), sound
checks.append('sound bank virtual methods and growth')
assert words('active_stream_list', 4) == (0, 0, 0, 0)
checks.append('active stream list')
print('TITBIT_INITIALIZER_RESULT ' + json.dumps({'ok': True, 'checks': checks}))
end
kill
quit
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, default=ROOT/'build/titbit')
    args = parser.parse_args()
    if not shutil.which('gdb'):
        parser.error('Install gdb for this optional startup check.')
    executable = args.executable.resolve()
    if not executable.is_file():
        parser.error('Build the native game first: '+str(executable))
    output = ROOT/'build/initializer-test'
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='titbit-initializers-') as temporary:
        commands = Path(temporary)/'check.gdb'
        commands.write_text(GDB_CHECK)
        environment = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        result = subprocess.run(['gdb', '-q', '--batch', '-x', str(commands), str(executable)],
                                capture_output=True, text=True, env=environment, timeout=30)
    (output/'gdb.log').write_text(result.stdout + result.stderr)
    rows = [line for line in result.stdout.splitlines() if line.startswith('TITBIT_INITIALIZER_RESULT ')]
    report = json.loads(rows[-1].split(' ', 1)[1]) if rows else {'ok': False}
    report['exitCode'] = result.returncode
    report['ok'] = report['ok'] and result.returncode == 0
    (output/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
