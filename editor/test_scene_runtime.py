import unittest

from formats import parse_scene
from scene_runtime import scene_animations, on_create, strip_comments
from server import Application


class SceneRuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = Application()

    def test_bomj_scene_has_full_background_and_authored_idle(self):
        scene = self.app.scene('getto_bomj_n.xml')
        background = next(o for o in scene['objects'] if o['name'] == 'Background')
        mesh = self.app.mesh(background['lods'][0]['meatname'])
        material = self.app.material(mesh['materialNames'][0])
        self.assertFalse(material['blend'])
        self.assertEqual(material['textures'][0].lower(), 'getto_bomj_with_alpha1.tga')
        refs = scene_animations(self.app.archive, scene)['objects']
        bomj = next(o for o in refs if o['objectName'] == 'bomj')
        self.assertEqual(bomj['clips'][0]['name'].lower(), 'bomj_idle.eza')
        self.assertTrue(bomj['clips'][0]['initial'])
        self.assertIn('bomj_talk.eza', [c['name'].lower() for c in bomj['clips']])
        anton = next(o for o in refs if o['objectName'] == 'anton')
        self.assertTrue(anton['clips'][0]['initial'])
        self.assertEqual(anton['clips'][0]['name'].lower(), 'gb_an_idle.eza')

    def test_entry_flag_two_is_discovered_not_mistaken_for_missing(self):
        refs = self.app.animations('podjezd.XML')['objects']
        anton = next(o for o in refs if o['objectName'] == 'anton')
        self.assertTrue(anton['clips'][0]['initial'])
        self.assertIn('_idle.', anton['clips'][0]['name'].lower())

    def test_custom_scene_uses_edited_current_model(self):
        scene = parse_scene(self.app.archive.read('getto_bomj_n.xml'), 'custom.xml')
        self.assertTrue(scene_animations(self.app.archive, scene)['objects'])

    def test_source_comments_and_on_create_boundaries(self):
        text = '/* self->CreateSkeletonAnimation("bad.eza",1); */ void ON_CREATE(){ if(1){x="}";} self->CreateSkeletonAnimation("good.eza",2); } void ON_NOTIFY(){}'
        block = on_create(strip_comments(text))
        self.assertIn('good.eza', block)
        self.assertNotIn('ON_NOTIFY', block)
        self.assertNotIn('bad.eza', block)


if __name__ == '__main__':
    unittest.main()
