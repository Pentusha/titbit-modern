"""Exercise engine/environment boundaries without requiring a display server."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TEMPLATE = Path(sys.argv.pop(1)).read_text()
SHELL = sys.argv.pop(1)
MODULE_VARIABLES = (
    "GIO_EXTRA_MODULES", "GIO_MODULE_DIR", "GDK_PIXBUF_MODULE_FILE",
    "GDK_PIXBUF_MODULEDIR", "GTK_PATH", "GTK_MODULES", "GTK_IM_MODULE_FILE",
)


class LauncherTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.data = root / "game data"
        self.data.mkdir()
        (self.data / "QUEST.ezd").touch()
        self.env = dict(os.environ)
        for key in ("SDL_VIDEODRIVER", "DISPLAY", "WAYLAND_DISPLAY"):
            self.env.pop(key, None)
        self.env.update({name: "/native/64-bit/plugins" for name in MODULE_VARIABLES})
        self.env.update(TITBIT_GAME=str(self.data), DISPLAY=":0", WAYLAND_DISPLAY="wayland-0")
        stub = f"""#!{sys.executable}
import json, os, sys
print(json.dumps(dict(role=os.path.basename(sys.argv[0]), args=sys.argv[1:], env=dict(os.environ))))
"""
        for name in ("engine", "setup"):
            path = root / name
            path.write_text(stub)
            path.chmod(0o755)
        launcher = TEMPLATE.replace("@shell@", SHELL)
        launcher = launcher.replace("@engine@", str(root / "engine"))
        launcher = launcher.replace("@setup@", str(root / "setup"))
        self.launcher = root / "titbit"
        self.launcher.write_text(launcher)
        self.launcher.chmod(0o755)

    def run_launcher(self, *args):
        result = subprocess.run([str(self.launcher), *args], env=self.env,
                                capture_output=True, text=True, check=True)
        return json.loads(result.stdout)

    def test_default_launch_cleans_modules_and_preserves_data_path(self):
        result = self.run_launcher()
        self.assertEqual(result["args"], ["--data", str(self.data)])
        self.assertEqual(result["env"]["SDL_VIDEODRIVER"], "x11")
        self.assertTrue(set(MODULE_VARIABLES).isdisjoint(result["env"]))

    def test_explicit_arguments_also_clean_engine_environment(self):
        result = self.run_launcher("--data", "custom directory", "--help")
        self.assertEqual(result["args"], ["--data", "custom directory", "--help"])
        self.assertTrue(set(MODULE_VARIABLES).isdisjoint(result["env"]))

    def test_setup_retains_native_desktop_environment(self):
        (self.data / "QUEST.ezd").unlink()
        result = self.run_launcher()
        self.assertEqual(result["role"], "setup")
        for name in MODULE_VARIABLES:
            self.assertEqual(result["env"][name], self.env[name])
        self.assertNotIn("SDL_VIDEODRIVER", result["env"])

    def test_explicit_wayland_driver_is_preserved(self):
        self.env["SDL_VIDEODRIVER"] = "wayland"
        self.assertEqual(self.run_launcher()["env"]["SDL_VIDEODRIVER"], "wayland")

    def test_wayland_only_session_keeps_automatic_driver_selection(self):
        self.env.pop("DISPLAY")
        self.assertNotIn("SDL_VIDEODRIVER", self.run_launcher()["env"])


if __name__ == "__main__":
    unittest.main()
