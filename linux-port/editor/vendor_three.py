"""Fetch a pinned Three.js distribution once; the editor runs fully offline."""
import base64
import hashlib
import io
import json
import pathlib
import re
import tarfile
import urllib.request

VERSION = '0.186.1'
ROOT = pathlib.Path(__file__).resolve().parent


def main():
    url = 'https://registry.npmjs.org/three/' + VERSION
    with urllib.request.urlopen(url, timeout=30) as response:
        meta = json.load(response)
    with urllib.request.urlopen(meta['dist']['tarball'], timeout=60) as response:
        payload = response.read()
    integrity = 'sha512-' + base64.b64encode(hashlib.sha512(payload).digest()).decode()
    if integrity != meta['dist']['integrity']:
        raise ValueError('Three.js package integrity differs')
    with tarfile.open(fileobj=io.BytesIO(payload), mode='r:gz') as tar:
        pending = ['build/three.module.js', 'build/three.core.js', 'LICENSE',
                   'examples/jsm/controls/OrbitControls.js', 'examples/jsm/controls/TransformControls.js',
                   'examples/jsm/loaders/OBJLoader.js', 'examples/jsm/loaders/GLTFLoader.js',
                   'examples/jsm/loaders/MTLLoader.js', 'examples/jsm/exporters/GLTFExporter.js']
        done = set()
        while pending:
            name = pending.pop()
            if name in done:
                continue
            done.add(name)
            data = tar.extractfile('package/' + name).read()
            relative = name.removeprefix('build/').replace('examples/jsm/', 'addons/')
            out = ROOT/'web/vendor'/relative
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(data)
            if name.endswith('.js'):
                for dep in re.findall(r'(?:from\s*|import\s*)[\'\"]([^\'\"]+)[\'\"]', data.decode()):
                    if dep.startswith('.'):
                        import posixpath
                        target = posixpath.normpath(posixpath.join(posixpath.dirname(name), dep))
                        if target.startswith(('examples/jsm/', 'build/')):
                            pending.append(target)
        (ROOT/'web/vendor/package-info.json').write_text(json.dumps({
            'package': 'three', 'version': VERSION, 'integrity': integrity,
            'source': meta['dist']['tarball'], 'files': sorted(done)}, indent=2), encoding='utf-8')
        print('Vendored Three.js', VERSION, len(done), 'files')


if __name__ == '__main__':
    main()
