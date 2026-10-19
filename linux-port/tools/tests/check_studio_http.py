"""Exercise the Linux launcher and real scene/mesh/texture/animation HTTP routes."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    archive = args.archive.resolve()
    digest_before = hashlib.sha256(archive.read_bytes()).hexdigest()
    with tempfile.TemporaryDirectory(prefix='titbit-studio-http-') as temporary:
        process = subprocess.Popen([sys.executable, str(root/'tools/start_studio.py'), '--archive', str(archive),
                                    '--port', '0', '--no-browser', '--projects', temporary],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        ready = threading.Event()
        first_line = []
        def read_ready():
            first_line.append(process.stdout.readline())
            ready.set()
        threading.Thread(target=read_ready, daemon=True).start()
        try:
            if not ready.wait(40) or not first_line[0].startswith('Titbit Studio http://127.0.0.1:'):
                raise RuntimeError('Launcher did not become ready: '+repr(first_line))
            base = first_line[0].split()[2]
            def request(route, name=None):
                url = base + route
                if name is not None:
                    url += '?'+urllib.parse.urlencode({'name': name})
                with urllib.request.urlopen(url, timeout=25) as response:
                    return response.read(), response.headers.get_content_type()
            catalog = json.loads(request('api/catalog')[0])
            assert len(catalog['scenes']) == 70, len(catalog['scenes'])
            for scene in catalog['scenes']:
                content = json.loads(request('api/scene', scene['name'])[0])
                assert content['name'] == scene['name']
            assert b'<html' in request('')[0].lower()
            counts = {}
            for extension, route in (('.meat', 'mesh'), ('.eza', 'animation'), ('.tga', 'texture')):
                asset = next(item for item in catalog['assets'] if item['kind'] == extension)
                content, kind = request('api/'+route, asset['name'])
                if route == 'texture':
                    assert kind == 'image/png'
                    with Image.open(io.BytesIO(content)) as image:
                        image.verify()
                else:
                    assert isinstance(json.loads(content), dict)
                counts[route] = {'asset': asset['name'], 'bytes': len(content)}
            saved = {'version': 1, 'scenes': {}, 'importedAssets': {}}
            with urllib.request.urlopen(urllib.request.Request(base+'api/project', data=json.dumps(saved).encode(),
                    headers={'Content-Type': 'application/json'}, method='POST'), timeout=10) as response:
                assert json.load(response)['ok']
            assert json.loads((Path(temporary)/'default.json').read_text()) == saved
            try:
                urllib.request.urlopen(urllib.request.Request(base+'api/catalog', headers={'Origin': 'https://example.com'}))
                raise AssertionError('Cross-origin request was allowed')
            except urllib.error.HTTPError as exc:
                assert exc.code == 403
            digest_after = hashlib.sha256(archive.read_bytes()).hexdigest()
            assert digest_before == digest_after
            result = {'ok': True, 'python': sys.version.split()[0], 'platform': sys.platform,
                      'scenes': len(catalog['scenes']), 'assets': len(catalog['assets']), 'routes': counts,
                      'projectSave': True, 'crossOriginRefused': True, 'archiveSHA256': digest_after}
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')
            print(json.dumps(result, ensure_ascii=False, indent=2))
        finally:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            process.stdout.close()
            process.stderr.close()


if __name__ == '__main__':
    main()
