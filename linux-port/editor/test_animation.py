"""Real original EZA coverage and source-equation animation sampling checks."""
import math
import os
import pathlib
import struct
import unittest

try:
    from .animation import euler_quaternion, parse_eza, sample_pose, slerp
    from .formats import Archive, FormatError
except ImportError:
    from animation import euler_quaternion, parse_eza, sample_pose, slerp
    from formats import Archive, FormatError


ORIGINAL = (pathlib.Path(os.environ['TITBIT_GAME'])/'QUEST.ezd' if os.environ.get('TITBIT_GAME')
            else pathlib.Path(__file__).resolve().parents[1]/"decompilation/original/game/App_Executables/QUEST.ezd")


def clip_bytes(values, frame_time=1.0, position_channels=(0, -1, -1), rotation_channels=(-1, -1, 1), bone=0):
    channels = len(values[0])
    header = b"EZA!_1.0\0"+struct.pack("<i", 1)
    track = struct.pack("<iii32s8i", 1, 2, 3, b"Bip01", bone, -1, *position_channels, *rotation_channels)
    flat = [value for row in values for value in row]
    return header+track+struct.pack("<iif", channels, len(values), frame_time)+struct.pack("<%df" % len(flat), *flat)


def quat_rotate(quat, vector):
    x, y, z, w = quat
    vx, vy, vz = vector
    # Quaternion rotation written independently of animation implementation.
    tx, ty, tz = 2*(y*vz-z*vy), 2*(z*vx-x*vz), 2*(x*vy-y*vx)
    return [vx+w*tx+(y*tz-z*ty), vy+w*ty+(z*tx-x*tz), vz+w*tz+(x*ty-y*tx)]


class AnimationTests(unittest.TestCase):
    def test_layout_metadata_and_length_validation(self):
        raw = clip_bytes([[0, 0], [10, 90]])
        clip = parse_eza(raw, "move.eza")
        self.assertEqual((clip["fps"], clip["duration"], clip["frameCount"], clip["channelCount"]), (1, 1, 2, 2))
        self.assertEqual(clip["tracks"][0]["positionChannels"], [0, -1, -1])
        self.assertEqual(clip["frames"], [0, 0, 10, 90])
        for invalid in (raw[:-1], raw+b"extra", b"EZA???", clip_bytes([[0, 0], [1, 1]], frame_time=0),
                        clip_bytes([[0, 0], [1, float("nan")]]),
                        clip_bytes([[0, 0], [1, 1]], position_channels=(99, -1, -1)),
                        clip_bytes([[0, 0], [1, 1]], bone=-1)):
            with self.assertRaises(FormatError):
                parse_eza(invalid)

    def test_intermediate_pose_positions_and_quaternion(self):
        clip = parse_eza(clip_bytes([[0, 0], [10, 90]]))
        bones = [dict(id=0, parent=-1, position=[99, 7, 8], rotation=[20, 30, 40], scale=[2, 3, 4]),
                 dict(id=1, parent=0, position=[2, 4, 6], rotation=[1, 2, 3], scale=[1, 1, 1])]
        start = sample_pose(clip, 0, bones)
        self.assertEqual(start["bones"][0]["position"], [0, 7, 8])
        self.assertEqual(start["bones"][0]["scale"], [2, 3, 4])
        self.assertEqual(start["bones"][0]["quaternion"], [0, 0, 0, 1])
        self.assertEqual(start["bones"][1], bones[1])
        middle = sample_pose(clip, 0.5, bones)
        self.assertEqual(middle["bones"][0]["position"], [5, 7, 8])
        vector = quat_rotate(middle["bones"][0]["quaternion"], [1, 0, 0])
        self.assertAlmostEqual(vector[0], math.sqrt(0.5), places=6)
        self.assertAlmostEqual(vector[1], math.sqrt(0.5), places=6)
        self.assertEqual(bones[0]["position"], [99, 7, 8], "bind data must not be mutated")

    def test_native_loop_endpoint_and_editor_endpoint_clamp(self):
        clip = parse_eza(clip_bytes([[0, 0], [10, 90], [20, 180]]))
        self.assertEqual(clip["duration"], 2)
        self.assertEqual(sample_pose(clip, 2)["bones"][0]["position"][0], 0)
        self.assertEqual(sample_pose(clip, 2, loop=False)["bones"][0]["position"][0], 20)
        self.assertEqual(sample_pose(clip, 20, loop=False)["frame"], 2)
        self.assertEqual(sample_pose(clip, 2.5)["bones"][0]["position"][0], 5)
        self.assertEqual(sample_pose(clip, -1)["time"], 0)

    def test_shortest_arc_and_original_linear_fallback(self):
        clip = parse_eza(clip_bytes([[0, 350], [0, 10]]))
        vector = quat_rotate(sample_pose(clip, 0.5)["bones"][0]["quaternion"], [1, 0, 0])
        self.assertAlmostEqual(vector[0], 1, places=6)
        self.assertAlmostEqual(vector[1], 0, places=6)
        # The game uses direct linear fallback without adding normalization.
        a, b = [0, 0, 0, 1], [0, 0, 0.01, math.sqrt(1-0.0001)]
        q = slerp(a, b, 0.5)
        self.assertAlmostEqual(q[2], 0.005, places=8)
        self.assertLess(sum(value*value for value in q), 1)

    def test_euler_composition_is_z_y_x(self):
        q = euler_quaternion([90, 90, 0])
        vector = quat_rotate(q, [0, 1, 0])
        # Rotate X first: Y -> Z; then Y: Z -> X.
        self.assertAlmostEqual(vector[0], 1, places=6)
        self.assertAlmostEqual(vector[1], 0, places=6)
        self.assertAlmostEqual(vector[2], 0, places=6)

    def test_unmatched_bone_is_skipped_and_absent_rotation_resets(self):
        clip = parse_eza(clip_bytes([[2, 3], [4, 5]], rotation_channels=(-1, -1, -1)))
        self.assertEqual(sample_pose(clip, 0, [dict(id=1, position=[1, 2, 3])])["bones"], [dict(id=1, position=[1, 2, 3])])
        pose = sample_pose(clip, 0, [dict(id=0, position=[1, 2, 3], rotation=[90, 0, 0], scale=[1, 2, 3])])["bones"][0]
        self.assertEqual(pose["quaternion"], [0, 0, 0, 1])
        self.assertEqual(pose["position"], [2, 2, 3])


@unittest.skipUnless(ORIGINAL.exists(), "Original archive is not installed")
class OriginalAnimationTests(unittest.TestCase):
    def test_every_original_eza_parses_and_samples_finite_local_poses(self):
        archive = Archive(ORIGINAL)
        names = list(dict.fromkeys(name.lower() for name in archive.names() if name.lower().endswith(".eza")))
        self.assertEqual(len(names), 455)
        rates = {}
        for name in names:
            clip = parse_eza(archive.read(name), name)
            rounded_rate = round(clip["fps"])
            rates[rounded_rate] = rates.get(rounded_rate, 0)+1
            self.assertGreaterEqual(clip["frameCount"], 2, name)
            for time in (0, clip["duration"]*0.37, clip["duration"]):
                pose = sample_pose(clip, time, loop=False)
                for bone in pose["bones"]:
                    self.assertTrue(all(math.isfinite(value) for value in bone["position"]+bone["quaternion"]), name)
                    self.assertAlmostEqual(sum(value*value for value in bone["quaternion"]), 1, delta=0.0011, msg=name)
            self.assertAlmostEqual(sample_pose(clip, clip["duration"])["time"], 0, msg=name)
        self.assertEqual(rates, {10.0: 450, 30.0: 4, 20.0: 1})
        print("Original EZA coverage:", len(names), "clips; rates:", rates)


if __name__ == "__main__":
    unittest.main()
