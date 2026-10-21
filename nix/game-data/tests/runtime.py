"""Exercise automatic setup with the real importer and synthetic game data."""

import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


class GameDataTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.data = self.root / "user data" / "titbit" / "game"
        self.calls = self.root / "game-args.json"
        self.env = dict(os.environ, HOME=str(self.root), XDG_DATA_HOME=str(self.root / "user data"),
                        TITBIT_TEST_ARGS=str(self.calls))
        self.env.pop("TITBIT_GAME", None)

    def launch(self, *args):
        return subprocess.run([os.environ["TITBIT_WRAPPER"], *args], env=self.env,
                              text=True, capture_output=True, check=False)

    def test_first_launch_copies_writable_data_and_passes_data_path(self):
        result = self.launch()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(list(self.data.iterdir())), 19)
        self.assertTrue((self.data / "QUEST.ezd").stat().st_mode & 0o200)
        self.assertEqual(json.loads(self.calls.read_text()), ["--data", str(self.data)])
        self.assertEqual((self.data / "QUEST.ezd").read_bytes(),
                         (Path(os.environ["TITBIT_DATA"]) / "QUEST.ezd").read_bytes())

    def test_repeat_launch_preserves_saves_and_modified_archive(self):
        self.assertEqual(self.launch().returncode, 0)
        (self.data / "savegame.dat").write_bytes(b"keep my save")
        (self.data / "QUEST.ezd").write_bytes(b"modified archive")
        result = self.launch()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.data / "savegame.dat").read_bytes(), b"keep my save")
        self.assertEqual((self.data / "QUEST.ezd").read_bytes(), b"modified archive")

    def test_help_does_not_create_user_data(self):
        result = self.launch("--help")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.data.exists())
        self.assertIn("fixture game help", result.stdout)
        self.assertFalse(self.calls.exists())

    def test_invalid_data_argument_does_not_import_or_launch(self):
        for args in (("--data",), ("--data", ""), ("--data=",)):
            with self.subTest(args=args):
                self.assertEqual(self.launch(*args).returncode, 2)
                self.assertFalse(self.data.exists())
                self.assertFalse(self.calls.exists())

    def test_explicit_data_path_with_spaces(self):
        for style in ("separate", "equals", "environment"):
            with self.subTest(style=style):
                dest = self.root / (style + " data")
                if style == "separate":
                    result = self.launch("--data", str(dest))
                elif style == "equals":
                    result = self.launch("--data=" + str(dest))
                else:
                    self.env["TITBIT_GAME"] = str(dest)
                    result = self.launch()
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue((dest / "QUEST.ezd").exists())
                self.assertEqual(json.loads(self.calls.read_text()), ["--data", str(dest)])

    def test_failed_import_keeps_existing_files_and_does_not_launch_game(self):
        self.data.mkdir(parents=True)
        (self.data / "clip00.avi").write_bytes(b"modified")
        (self.data / "savegame.dat").write_bytes(b"keep my save")
        result = self.launch()
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.calls.exists())
        self.assertEqual(sorted(p.name for p in self.data.iterdir()), ["clip00.avi", "savegame.dat"])
        self.assertEqual((self.data / "savegame.dat").read_bytes(), b"keep my save")

    def test_source_selection_rejects_ambiguous_matching_images(self):
        spec = importlib.util.spec_from_file_location("prepare", os.environ["TITBIT_PREPARE"])
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        source = self.root / "multiple discs"
        source.mkdir()
        iso = Path(os.environ["TITBIT_FIXTURE"]) / "torrent" / "disc.iso"
        shutil.copy2(iso, source / "one.iso")
        shutil.copy2(iso, source / "two.iso")
        with self.assertRaisesRegex(ValueError, "Several matching"):
            module.select_source(os.environ["TITBIT_IMPORTER"], source)

    def test_cue_ccd_and_img_of_one_disc_import_as_one_source(self):
        source = Path(os.environ["TITBIT_FIXTURE"]) / "clonecd"
        result = subprocess.run(
            [sys.executable, os.environ["TITBIT_PREPARE"],
             "--importer", os.environ["TITBIT_IMPORTER"],
             "--source", str(source), "--dest", str(self.data)],
            text=True, capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(list(self.data.iterdir())), 19)
        self.assertEqual((self.data / "QUEST.ezd").read_bytes(),
                         (Path(os.environ["TITBIT_DATA"]) / "QUEST.ezd").read_bytes())


if __name__ == "__main__":
    unittest.main()
