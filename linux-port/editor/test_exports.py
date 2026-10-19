import copy
import io
import pathlib
import tempfile
import unittest
import zipfile

from formats import Archive, parse_scene, parse_meat, parse_mat
from server import Application, GAME, validate_project
from exporters import native_replacements, obj_zip, bundle_zip, bone_world, matrix, mul, decompose_bone


class ExportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = Application()

    def scene(self):
        return copy.deepcopy(self.app.scene('getto_skamejka.XML'))

    def test_lod_spaces_material_suffix(self):
        self.assertGreater(len(self.app.mesh('rap1.MEAT ')['parts']), 0)
        self.assertTrue(self.app.material('Getto_center')['textures'])

    def test_native_transform_preserves_skeleton(self):
        scene = self.scene()
        original = bone_world(scene['objects'][1])
        delta = {'position': [3, 7, -2], 'rotation': [0, 0, 30], 'scale': [2, 2, 2]}
        scene['objects'][1]['editorTransform'] = delta
        files = native_replacements(self.app.archive, scene)
        saved = parse_scene(files[scene['name']], scene['name'])
        result = bone_world(saved['objects'][1])
        self.assertEqual(len(result), len(original))
        for key, value in original.items():
            expected = mul(matrix(delta), value)
            for a, b in zip(expected, result[key]):
                self.assertAlmostEqual(a, b, places=5)

    def test_native_refuses_shear(self):
        bone = {'rotation': [15, 35, 25], 'scale': [1, 2, 3]}
        with self.assertRaisesRegex(ValueError, 'shear'):
            decompose_bone(mul(matrix({'rotation': [0, 0, 45]}), matrix(bone, True)))

    def test_imported_mesh_native_materials(self):
        scene = self.scene()
        scene['objects'].append({'id': 'imported-test', 'name': 'Imported', 'group': 'Импортированные',
            'attributes': {}, 'bones': [], 'lods': [], 'xml': '', 'editorImported': True,
            'editorTransform': {'position': [1, 2, 3]},
            'editorMesh': {'materials': [{'name': 'red', 'color': [1, 0, 0]}], 'parts': [{
                'name': 'tri', 'positions': [0, 0, 0, 1, 0, 0, 0, 1, 0], 'indices': [0, 1, 2],
                'uv': [0, 0, 1, 0, 0, 1], 'uvLayout': 'vertex', 'materialNames': ['red'], 'faceMaterials': [0]}]}})
        files = native_replacements(self.app.archive, scene)
        saved = parse_scene(files[scene['name']], scene['name'])
        imported = saved['objects'][-1]
        self.assertEqual(imported['bones'][0]['position'], [1, 2, 3])
        mesh = parse_meat(files[imported['lods'][0]['meatname']])
        mat_name = mesh['materialNames'][0]+'.mat'
        self.assertIn(mat_name, files)
        material = parse_mat(files[mat_name])
        self.assertIn(material['textures'][0], files)
        self.assertEqual(mesh['parts'][0]['vertexBones'], [0, 0, 0])

    def test_obj_has_materials_textures_and_all_meshes(self):
        payload = obj_zip(self.app.archive, self.app.scene('getto_center.XML'))
        with zipfile.ZipFile(io.BytesIO(payload)) as z:
            text = z.read('scene.obj').decode()
            self.assertIn('Getto_center', text)
            self.assertIn('rapper1', text)
            self.assertIn('rapper2', text)
            self.assertIn('usemtl', text)
            self.assertGreater(sum(line.startswith('f ') for line in text.splitlines()), 5000)
            self.assertTrue(any(name.endswith('.png') for name in z.namelist()))

    def test_bundle_archive_reopens_and_preserves_original_asset(self):
        scene = self.scene()
        scene['objects'][0]['editorTransform'] = {'position': [1, 0, 0]}
        payload = bundle_zip(self.app.archive, scene)
        with tempfile.TemporaryDirectory() as directory:
            with zipfile.ZipFile(io.BytesIO(payload)) as z:
                path = pathlib.Path(directory)/'QUEST.ezd'
                path.write_bytes(z.read('QUEST.ezd'))
                rebuilt = Archive(path)
                self.assertEqual(rebuilt.read('anton.meat'), self.app.archive.read('anton.meat'))
                saved = parse_scene(rebuilt.read(scene['name']), scene['name'])
                a, b = self.scene()['objects'][0], saved['objects'][0]
                self.assertAlmostEqual(a['bones'][0]['position'][0]+1, b['bones'][0]['position'][0], places=5)

    def test_project_validation(self):
        scene = self.scene()
        validate_project({'version': 1, 'scenes': {scene['name']: scene}})
        scene['objects'].append(copy.deepcopy(scene['objects'][0]))
        with self.assertRaises(ValueError):
            validate_project({'version': 1, 'scenes': {scene['name']: scene}})

    def test_new_scene_export_uses_retained_original_xml(self):
        scene = self.scene()
        scene['name'] = 'test_custom.xml'
        files = native_replacements(self.app.archive, scene)
        self.assertIn('test_custom.xml', files)
        self.assertEqual(len(parse_scene(files['test_custom.xml'])['objects']), len(scene['objects']))

    def test_model_replacement_preserves_script_bone_and_components(self):
        scene = self.scene()
        original = copy.deepcopy(scene['objects'][-1])
        obj = scene['objects'][-1]
        obj.update(editorReplace=True, editorImported=False, editorMesh={
            'parts': [{'name': 'tri', 'positions': [0, 0, 0, 1, 0, 0, 0, 1, 0], 'indices': [0, 1, 2],
                       'materialNames': ['default'], 'faceMaterials': [0]}]})
        files = native_replacements(self.app.archive, scene)
        saved = parse_scene(files[scene['name']], scene['name'])['objects'][-1]
        self.assertEqual(saved['attributes'], original['attributes'])
        for key in ('position', 'rotation', 'scale'):
            for a, b in zip(saved['bones'][0][key], original['bones'][0][key]):
                self.assertAlmostEqual(a, b, places=5)
        self.assertNotEqual(saved['lods'][0]['meatname'], original['lods'][0]['meatname'])


if __name__ == '__main__':
    unittest.main()
