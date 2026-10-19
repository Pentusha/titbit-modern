"""Compatibility entry point; use tools/start_studio.sh on Linux."""
from pathlib import Path
import runpy

if __name__ == '__main__':
    runpy.run_path(str(Path(__file__).resolve().parents[1] / 'tools' / 'start_studio.py'), run_name='__main__')
