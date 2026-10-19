"""Local-only scene editor. Original game archive is opened for reading only."""
import argparse
import functools
import io
import json
import mimetypes
import os
import pathlib
import threading
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from PIL import Image
from formats import Archive, parse_meat, parse_mat, parse_scene
from exporters import obj_zip, bundle_zip
from animation import parse_eza
from scene_runtime import scene_animations

ROOT = pathlib.Path(__file__).resolve().parent
GAME = (pathlib.Path(os.environ['TITBIT_GAME'])/'QUEST.ezd' if os.environ.get('TITBIT_GAME')
        else ROOT.parent/'decompilation/original/game/App_Executables/QUEST.ezd')
PROJECT = ROOT/'projects/default.json'
LOCK = threading.Lock()


def dumps(value):
    return json.dumps(value, ensure_ascii=False, allow_nan=False).encode('utf-8')


def load_project():
    if PROJECT.exists():
        return json.loads(PROJECT.read_text(encoding='utf-8'))
    return {'version': 1, 'scenes': {}, 'importedAssets': {}}


def validate_project(data):
    if not isinstance(data, dict) or data.get('version') != 1 or not isinstance(data.get('scenes'), dict):
        raise ValueError('Invalid project format')
    if len(data['scenes']) > 150:
        raise ValueError('Too many scenes')
    for name, scene in data['scenes'].items():
        validate_scene(scene)
        if scene['name'] != name:
            raise ValueError('Scene key differs from its name')
    dumps(data)


def validate_scene(scene):
    if not isinstance(scene, dict) or not isinstance(scene.get('objects'), list):
        raise ValueError('Invalid scene')
    name = scene.get('name', '')
    if not isinstance(name, str) or not name.lower().endswith('.xml') or any(c in name for c in ('..', '/', '\\', '\0')):
        raise ValueError('Invalid scene filename')
    ids = set()
    if len(scene['objects']) > 10000:
        raise ValueError('Too many scene objects')
    for obj in scene['objects']:
        if not isinstance(obj.get('id'), str) or obj['id'] in ids:
            raise ValueError('Object IDs must be unique strings')
        ids.add(obj['id'])
        for key in ('editorTransform',):
            transform = obj.get(key, {})
            for component in ('position', 'rotation', 'scale'):
                if component in transform:
                    values = transform[component]
                    if not isinstance(values, list) or len(values) != 3 or any(type(x) not in (float, int) for x in values):
                        raise ValueError('Invalid transform')
    dumps(scene)


class Application:
    def __init__(self, archive_path=GAME):
        self.archive = Archive(archive_path)
        self.assets = []
        self.scenes = []
        seen = set()
        for entry in reversed(self.archive.entries):
            if entry['name'].lower() in seen:
                continue
            seen.add(entry['name'].lower())
            self.assets.append({'name': entry['name'], 'size': entry['size'], 'kind': pathlib.Path(entry['name']).suffix.lower()})
            if entry['name'].lower().endswith('.xml'):
                scene = self.scene(entry['name'])
                self.scenes.append({'name': entry['name'], 'objects': len(scene['objects']), 'camera': scene.get('camera', {}).get('present', True)})
        self.assets.sort(key=lambda a: a['name'].lower())
        self.scenes.sort(key=lambda a: a['name'].lower())

    @functools.lru_cache(maxsize=100)
    def scene(self, name):
        return parse_scene(self.archive.read(name), name)

    @functools.lru_cache(maxsize=350)
    def mesh(self, name):
        # Lod::Load_004153b0 removes ASCII spaces before archive lookup.
        name = name.replace(' ', '')
        return parse_meat(self.archive.read(name), name)

    @functools.lru_cache(maxsize=200)
    def material(self, name):
        if not name.lower().endswith('.mat'):
            name += '.mat'
        return parse_mat(self.archive.read(name), name)

    @functools.lru_cache(maxsize=100)
    def animations(self, name):
        return scene_animations(self.archive, self.scene(name))

    @functools.lru_cache(maxsize=30)
    def animation(self, name):
        name = name.replace(' ', '')
        return parse_eza(self.archive.read(name), name)

    @functools.lru_cache(maxsize=160)
    def texture(self, name):
        with Image.open(io.BytesIO(self.archive.read(name))) as image:
            if image.width*image.height > 32*1024*1024:
                raise ValueError('Texture too large')
            out = io.BytesIO(); image.convert('RGBA').save(out, 'PNG')
            return out.getvalue()


class Handler(BaseHTTPRequestHandler):
    server_version = 'TitbitStudio/1.0'

    def respond(self, content, kind='application/json; charset=utf-8', status=200, filename=None):
        self.send_response(status)
        self.send_header('Content-Type', kind)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Cache-Control', 'no-store' if self.path.startswith('/api/') else 'no-cache')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Referrer-Policy', 'same-origin')
        self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob:; connect-src 'self' blob: data:; worker-src 'self' blob:")
        if filename:
            self.send_header('Content-Disposition', 'attachment; filename="'+filename+'"')
        self.end_headers()
        self.wfile.write(content)

    def allowed(self):
        host = self.headers.get('Host', '')
        valid = {f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}'}
        origin = self.headers.get('Origin')
        if host not in valid or (origin and origin not in {'http://'+v for v in valid}):
            self.respond(dumps({'error': 'Only local same-origin editor requests are allowed'}), status=403)
            return False
        return True

    def do_GET(self):
        if not self.allowed():
            return
        try:
            url = urllib.parse.urlsplit(self.path)
            args = urllib.parse.parse_qs(url.query)
            name = args.get('name', [''])[0]
            app = self.server.app
            if url.path == '/api/catalog':
                self.respond(dumps({'scenes': app.scenes, 'assets': app.assets, 'project': load_project()}))
            elif url.path == '/api/project':
                self.respond(dumps(load_project()))
            elif url.path == '/api/scene':
                self.respond(dumps(app.scene(name)))
            elif url.path == '/api/mesh':
                self.respond(dumps(app.mesh(name)))
            elif url.path == '/api/material':
                self.respond(dumps(app.material(name)))
            elif url.path == '/api/animations':
                self.respond(dumps(app.animations(args.get('scene', [name])[0])))
            elif url.path == '/api/animation':
                self.respond(dumps(app.animation(name)))
            elif url.path == '/api/texture':
                self.respond(app.texture(name), 'image/png')
            elif url.path == '/api/asset':
                entry = app.archive.entry(name)
                self.respond(app.archive.read(name), 'application/octet-stream')
            elif url.path.startswith('/api/'):
                self.respond(dumps({'error': 'Unknown API route'}), status=404)
            else:
                relative = urllib.parse.unquote(url.path).lstrip('/') or 'index.html'
                path = (ROOT/'web'/relative).resolve()
                if not path.is_relative_to((ROOT/'web').resolve()) or not path.is_file():
                    self.respond(dumps({'error': 'Not found'}), status=404)
                    return
                kind = mimetypes.guess_type(path.name)[0] or 'application/octet-stream'
                if path.suffix == '.js':
                    kind = 'text/javascript; charset=utf-8'
                self.respond(path.read_bytes(), kind)
        except (KeyError, FileNotFoundError) as e:
            self.respond(dumps({'error': 'Asset not found: '+str(e)}), status=404)
        except (ValueError, OSError) as e:
            self.respond(dumps({'error': str(e)}), status=400)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_POST(self):
        if not self.allowed():
            return
        try:
            if self.headers.get_content_type() != 'application/json':
                raise ValueError('JSON required')
            size = int(self.headers.get('Content-Length', '0'))
            if size <= 0 or size > 64*1024*1024:
                raise ValueError('Project must fit within 64 MiB')
            body = json.loads(self.rfile.read(size), parse_constant=lambda v: (_ for _ in ()).throw(ValueError('Non-finite number')))
            if self.path == '/api/project':
                validate_project(body)
                data = dumps(body)
                with LOCK:
                    PROJECT.parent.mkdir(parents=True, exist_ok=True)
                    if PROJECT.exists():
                        PROJECT.with_suffix('.previous.json').write_bytes(PROJECT.read_bytes())
                    tmp = PROJECT.with_suffix('.tmp')
                    tmp.write_bytes(data)
                    os.replace(tmp, PROJECT)
                self.respond(dumps({'ok': True, 'path': 'editor/projects/default.json'}))
            elif self.path == '/api/animations':
                scene = body['scene']; validate_scene(scene)
                self.respond(dumps(scene_animations(self.server.app.archive, scene)))
            elif self.path == '/api/export':
                scene = body['scene']; validate_scene(scene)
                if body.get('format') == 'obj':
                    payload = obj_zip(self.server.app.archive, scene, body.get('object'))
                    filename = 'titbit-obj.zip'
                elif body.get('format') == 'bundle':
                    payload = bundle_zip(self.server.app.archive, scene)
                    filename = 'titbit-mod.zip'
                else:
                    raise ValueError('Unknown export format')
                self.respond(payload, 'application/zip', filename=filename)
            else:
                self.respond(dumps({'error': 'Unknown API route'}), status=404)
        except (KeyError, ValueError, OSError, TypeError, IndexError) as e:
            self.respond(dumps({'error': str(e)}), status=400)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default=4318, type=int)
    parser.add_argument('--archive', type=pathlib.Path, help='Path to your QUEST.ezd (read-only)')
    args = parser.parse_args()
    archive = args.archive or (pathlib.Path(os.environ['TITBIT_GAME'])/'QUEST.ezd' if os.environ.get('TITBIT_GAME') else GAME)
    app = Application(archive)
    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
    server.app = app
    print(f'Titbit Studio http://127.0.0.1:{args.port}/ ({len(app.scenes)} scenes)', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
