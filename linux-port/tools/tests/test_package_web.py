import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from package_web import RUNTIME, package


class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.build = self.root / 'build'
        self.build.mkdir()
        for name in RUNTIME:
            (self.build / name).write_bytes(name.encode())

    def test_only_runtime_is_published(self):
        for name in ('QUEST.ezd', 'intro.avi', 'slot1.sav', 'build.log', 'toolchain.json'):
            (self.build / name).write_bytes(b'private fixture')
        target = self.root / 'site'
        package(self.build, target)
        self.assertEqual({p.name for p in target.iterdir()}, set(RUNTIME) | {'index.html', '.nojekyll'})
        self.assertEqual((target / 'index.html').read_bytes(), (self.build / 'game.html').read_bytes())
        self.assertFalse(any(p.is_symlink() for p in target.iterdir()))

    def test_rejects_existing_output_or_build_directory(self):
        target = self.root / 'site'
        target.mkdir()
        (target / 'slot1.sav').write_bytes(b'private fixture')
        with self.assertRaises(ValueError):
            package(self.build, target)
        with self.assertRaises(ValueError):
            package(self.build, self.build / 'site')

    def test_missing_runtime_fails_before_creating_site(self):
        (self.build / 'titbit.wasm').unlink()
        target = self.root / 'site'
        with self.assertRaises(FileNotFoundError):
            package(self.build, target)
        self.assertFalse(target.exists())
