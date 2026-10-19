"""Format invariants and read-only round trips against actual original assets."""
import copy
import hashlib
import io
import os
import pathlib
import struct
import tempfile
import unittest
import zlib

try:
    from .formats import Archive, FormatError, apply_scene, parse_mat, parse_meat, parse_scene, write_meat
except ImportError:
    from formats import Archive, FormatError, apply_scene, parse_mat, parse_meat, parse_scene, write_meat


ROOT = pathlib.Path(__file__).resolve().parents[1]
ORIGINAL = (pathlib.Path(os.environ['TITBIT_GAME'])/'QUEST.ezd' if os.environ.get('TITBIT_GAME')
            else ROOT/"decompilation/original/game/App_Executables/QUEST.ezd")


def synthetic_archive(rows):
    rows = [(name, zlib.compress(data), len(data)) for name, data in rows]
    start = 16+sum(len(name.encode("cp1251"))+18 for name, _, _ in rows)
    total = start+sum(len(data) for _, data, _ in rows)
    out = bytearray(b"ezd\xff"+struct.pack("<III", len(rows), start, total))
    offset = start
    for name, data, plain in rows:
        out.extend(name.encode("cp1251")+b"\0\0"+struct.pack("<IIII", 2, offset, plain, len(data)))
        offset += len(data)
    return bytes(out)+b"".join(data for _, data, _ in rows)


class ArchiveTests(unittest.TestCase):
    def test_duplicates_last_wins_and_mod_rebuild_preserves_unchanged(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)/"original.ezd"
            raw = synthetic_archive([("A.txt", b"first"), ("keep.bin", b"untouched"), ("a.TXT", b"last")])
            path.write_bytes(raw)
            archive = Archive(path)
            self.assertEqual(archive.read("A.TXT"), b"last")
            self.assertEqual(archive.entry("a.txt")["index"], 2)
            rebuilt = archive.build({"a.txt": b"changed", "new.dat": b"new"})
            mod_path = pathlib.Path(folder)/"mod.ezd"
            mod_path.write_bytes(rebuilt)
            mod = Archive(mod_path)
            self.assertEqual(mod.read("a.txt"), b"changed")
            self.assertEqual(mod.read("new.dat"), b"new")
            self.assertEqual(mod.names()[:3], archive.names())
            for i in (0, 1):
                old, new = archive.entries[i], mod.entries[i]
                self.assertEqual(raw[old["offset"]:old["offset"]+old["packed"]],
                                 rebuilt[new["offset"]:new["offset"]+new["packed"]])
            self.assertEqual(path.read_bytes(), raw)
            out = io.BytesIO()
            self.assertEqual(archive.build_to(out, {"a.txt": b"changed", "new.dat": b"new"}), len(rebuilt))
            self.assertEqual(out.getvalue(), rebuilt)

    def test_truncated_index_and_bad_size_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)/"bad.ezd"
            for data in (b"ezd\xff", b"ezd\xff"+struct.pack("<III", 99, 16, 16)):
                path.write_bytes(data)
                with self.assertRaises(FormatError):
                    Archive(path)


def sample_mesh(version=20):
    return dict(version=version, slotCount=2, materialNames=["sample"], parts=[dict(
        name="test", positions=[0, 0, 0, 1, 0, 0, 0, 1, 0], normals=[0, 0, 1]*3,
        indices=[0, 1, 2], flags=7, vertexBones=[0, 1, 1], faceMaterials=[0],
        uv=[0, 0, 1, 0, 0, 1], uv2=[0, 0, 0.5, 0, 0, 0.5], uv3=[1, 1, 0, 1, 1, 0])])


class MeshTests(unittest.TestCase):
    def test_normals_uv_layers_bones_roundtrip_and_edit(self):
        for version in (20, 21):
            raw = write_meat(sample_mesh(version))
            parsed = parse_meat(raw)
            self.assertEqual(write_meat(parsed), raw)
            self.assertEqual(parsed["parts"][0]["vertexBones"], [0, 1, 1])
            self.assertEqual(parsed["parts"][0]["uv2"], sample_mesh()["parts"][0]["uv2"])
            parsed["parts"][0]["positions"][0] = 123
            self.assertEqual(parse_meat(write_meat(parsed))["parts"][0]["positions"][0], 123)

    def test_invalid_meshes_rejected(self):
        raw = write_meat(sample_mesh())
        for data in (raw[:-1], raw+b"unexpected", b"MEAT"+struct.pack("<I", 99)):
            with self.assertRaises(FormatError):
                parse_meat(data)
        mesh = sample_mesh()
        mesh["parts"][0]["indices"] = [0, 1, 99]
        with self.assertRaises(FormatError):
            write_meat(mesh)
        mesh = sample_mesh()
        mesh["parts"][0]["uv"] = [0]
        with self.assertRaises(FormatError):
            write_meat(mesh)


SCENE = b'''<!-- external header --><scene unknown="keep"><!-- inner -->
<camera fov="41.2"><position x="1" y="2" z="-5.70.2114"/><rotation x="90" y="0" z="0"/></camera>
<static_objects><object name="original" scriptname="test.scv" arbitrary="yes"><position owner_bone_id="0" bonecount="1"><bones><bone bone_id="0" radius="3"><position x="1" y="2" z="3"/><rotation x="0" y="0" z="0"/><scale x="1" y="1" z="1"/><unknown value="keep"/></bone></bones></position><lod meatname="test.meat"/><custom-component a="b"/><srb_lod meatname="test2.meat"/></object></static_objects>
<resources><keep value="yes"/></resources></scene><!-- external tail -->'''


class SceneTests(unittest.TestCase):
    def test_modify_duplicate_delete_unknowns_and_component_order(self):
        scene = parse_scene(SCENE, "test.xml")
        self.assertEqual(scene["camera"]["position"][2], -5.70)
        obj = scene["objects"][0]
        obj["bones"][0]["position"] = [5, 6, 7]
        obj["lods"][0]["meatname"] = "changed.meat"
        duplicate = copy.deepcopy(obj)
        duplicate.update(id="copy", name="duplicate", group="dynamic_objects")
        scene["objects"].append(duplicate)
        data = apply_scene(SCENE, scene)
        updated = parse_scene(data)
        self.assertEqual(len(updated["objects"]), 2)
        self.assertEqual(updated["objects"][0]["bones"][0]["position"], [5, 6, 7])
        self.assertEqual(updated["objects"][1]["group"], "dynamic_objects")
        for marker in (b"external header", b"external tail", b"<!-- inner -->", b"unknown", b"arbitrary", b"custom-component", b"resources", b"-5.70.2114"):
            self.assertIn(marker, data)
        self.assertEqual(updated["objects"][0]["components"], obj["components"])
        scene["objects"] = []
        self.assertFalse(parse_scene(apply_scene(SCENE, scene))["objects"])

    def test_missing_camera_not_added_and_metadata_byte_preserved(self):
        raw = b'<scene><static_objects><object name="x\x98"><lod meatname="a.meat"/></object></static_objects></scene>'
        scene = parse_scene(raw)
        data = apply_scene(raw, scene)
        self.assertIn(b"x\x98", data)
        self.assertNotIn(b"<camera", data)
        self.assertNotIn(b"<position", data)

    def test_reject_entities_and_nonfinite_editor_changes(self):
        with self.assertRaises(FormatError):
            parse_scene(b'<!DOCTYPE scene [<!ENTITY a SYSTEM "file:///x">]><scene/>')
        scene = parse_scene(SCENE)
        scene["objects"][0]["bones"][0]["position"] = [float("nan"), 0, 0]
        with self.assertRaises(FormatError):
            apply_scene(SCENE, scene)
        scene = parse_scene(SCENE)
        scene["objects"][0]["bones"][0]["parent"] = 0
        with self.assertRaises(FormatError):
            apply_scene(SCENE, scene)


@unittest.skipUnless(ORIGINAL.exists(), "Original archive is not installed")
class OriginalAssetTests(unittest.TestCase):
    def test_all_native_meshes_materials_scenes(self):
        archive = Archive(ORIGINAL)
        counts = dict(mesh=0, material=0, scene=0)
        for name in dict.fromkeys(n.lower() for n in archive.names()):
            suffix = pathlib.PureWindowsPath(name).suffix.lower()
            if suffix == ".meat":
                raw = archive.read(name)
                self.assertEqual(write_meat(parse_meat(raw, name)), raw, name)
                counts["mesh"] += 1
            elif suffix == ".mat":
                parse_mat(archive.read(name), name)
                counts["material"] += 1
            elif suffix == ".xml":
                raw = archive.read(name)
                scene = parse_scene(raw, name)
                updated = parse_scene(apply_scene(raw, scene), name)
                self.assertEqual(len(updated["objects"]), len(scene["objects"]), name)
                self.assertEqual([o["components"] for o in updated["objects"]], [o["components"] for o in scene["objects"]], name)
                counts["scene"] += 1
        self.assertGreaterEqual(counts["mesh"], 314)
        self.assertGreaterEqual(counts["material"], 161)
        self.assertGreaterEqual(counts["scene"], 70)
        print("Original read-only asset verification:", counts)


if __name__ == "__main__":
    unittest.main()
