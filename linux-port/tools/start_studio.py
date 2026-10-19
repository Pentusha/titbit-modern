#!/usr/bin/env python3
"""Run the local scene editor with an original QUEST.ezd archive."""
import argparse
import os
from pathlib import Path
import sys
import threading
import webbrowser

ROOT = Path(__file__).resolve().parents[1]
DATA_HOME = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'titbit'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, help='Original QUEST.ezd or its directory')
    parser.add_argument('--port', type=int, default=4318, help='Local port; 0 selects a free port')
    parser.add_argument('--projects', type=Path, default=DATA_HOME / 'studio-projects')
    parser.add_argument('--no-browser', action='store_true')
    args = parser.parse_args()
    candidates = [args.archive] if args.archive else [
        Path(os.environ['TITBIT_GAME']) if os.environ.get('TITBIT_GAME') else None,
        DATA_HOME / 'game', ROOT / 'game', Path.cwd()]
    archive = None
    for candidate in candidates:
        if candidate is None:
            continue
        if candidate.is_dir():
            candidate = next((candidate / name for name in ('QUEST.ezd', 'quest.ezd')
                              if (candidate / name).is_file()), candidate / 'QUEST.ezd')
        if candidate.is_file():
            archive = candidate.resolve()
            break
    if archive is None:
        parser.error('QUEST.ezd not found. Use --archive /path/to/QUEST.ezd or import your game first.')
    os.environ['TITBIT_STUDIO_PROJECTS'] = str(args.projects.expanduser().resolve())
    sys.path.insert(0, str(ROOT / 'editor'))
    try:
        import server
        app = server.Application(archive)
        httpd = server.ThreadingHTTPServer(('127.0.0.1', args.port), server.Handler)
    except (ImportError, ValueError, OSError) as exc:
        parser.exit(1, f'Studio: {exc}\n')
    httpd.app = app
    url = f'http://127.0.0.1:{httpd.server_port}/'
    print(f'Titbit Studio {url} ({len(app.scenes)} scenes)', flush=True)
    print(f'Archive (read only): {archive}\nProjects: {server.PROJECT.parent}', flush=True)
    if not args.no_browser:
        threading.Timer(0.3, lambda: webbrowser.open(url)).start()
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
