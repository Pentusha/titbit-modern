"""The Python resource compiler must keep writing what Windows SDK rc.exe wrote."""
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
from project_icon import write_icon
from rc_compile import RcError, compile_rc

# sha256 of game.res from rc.exe 10.0 (Windows SDK) for this script and the generated icon
RC_EXE_SHA256 = 'cb92432d7b589389d52a0be7915a668a08a3007b405ab22468fbfd9f0c0dc742'


class RcCompileTest(unittest.TestCase):
    def test_matches_rc_exe(self):
        source = TOOLS.parent / 'src/lanes/codex/resources/titbit.rc'
        text = source.read_text(encoding='utf-8')
        text = text.replace('icon_202_1049.ico', 'titbit-modern.ico').replace('icon_128_1033.ico', 'titbit-modern.ico')
        with tempfile.TemporaryDirectory() as tmp:
            write_icon(Path(tmp) / 'titbit-modern.ico')
            (Path(tmp) / 'titbit.rc').write_text(text, encoding='utf-8')
            data = compile_rc(Path(tmp) / 'titbit.rc')
        self.assertEqual(hashlib.sha256(data).hexdigest(), RC_EXE_SHA256)

    def test_rejects_unknown_statements(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'x.rc'
            path.write_text('1 BITMAP "x.bmp"\n', encoding='utf-8')
            with self.assertRaises(RcError):
                compile_rc(path)


if __name__ == '__main__':
    unittest.main()
