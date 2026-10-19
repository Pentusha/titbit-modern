"""Original EZA skeletal clips and a reference sampler for the web editor.

Evidence: restored engine42/animation.cpp, SkeletonAnimation::LoadFile 0x427300
and Update 0x426a20. File channels are absolute local positions and Euler
degrees. Animation never modifies scale; absent position axes retain the
owner's existing pose, whereas absent rotation axes become zero. Rotation is
qz*qy*qx, then shortest-arc SLERP(following, current, 1-alpha), as in the game.

The sampler follows the source equations with float32 output, not a claim of
bit-identical x87 intermediate rounding. Endpoints can be inspected without
simulating component destruction/events. Native looping wraps at duration;
loop=False clamps to the final file frame for editor scrubbing.
"""
from __future__ import annotations

import copy
import math
import struct

try:
    from .formats import FormatError
except ImportError:
    from formats import FormatError


_TRACK = struct.Struct("<iii32s8i")
_DEGREES_TO_RADIANS = struct.unpack("<f", struct.pack("<f", 0.017453292))[0]


def parse_eza(data, name="animation.eza"):
    """Return a JSON-safe frame-major channel table and explicit bone tracks.

    frames[frame * channelCount + channel] is one float. Track channel -1
    means absent. The first three record words/channel30 are preserved but are
    not used by the original runtime animation update. Name padding is ignored.
    """
    if len(data) < 25 or data[:9] != b"EZA!_1.0\0":
        raise FormatError("Not an EZA!_1.0 skeletal animation")
    count = struct.unpack_from("<i", data, 9)[0]
    if not 0 <= count <= 100_000:
        raise FormatError("Invalid EZA track count")
    end = 13+count*_TRACK.size
    if end+12 > len(data):
        raise FormatError("Truncated EZA track table")
    channels, frame_count, frame_time = struct.unpack_from("<iif", data, end)
    if not 0 <= channels <= 1_000_000 or not 0 <= frame_count <= 1_000_000:
        raise FormatError("Invalid EZA frame/channel count")
    value_count = channels*frame_count
    if value_count > 16_000_000 or end+12+value_count*4 != len(data):
        raise FormatError("EZA frame data length mismatch")
    if not math.isfinite(frame_time) or frame_time <= 0:
        raise FormatError("EZA frame time must be finite and positive")
    tracks = []
    for index in range(count):
        a, b, c, raw_name, bone, channel30, px, py, pz, rx, ry, rz = _TRACK.unpack_from(data, 13+index*_TRACK.size)
        if bone < 0 or bone > 1_000_000:
            raise FormatError("Invalid EZA bone ID")
        if any(channel != -1 and not 0 <= channel < channels for channel in (px, py, pz, rx, ry, rz)):
            raise FormatError("EZA track references a missing channel")
        bone_name = raw_name.split(b"\0", 1)[0].decode("cp1251", "surrogateescape").replace("\udc98", "\x98")
        tracks.append(dict(bone=bone, name=bone_name, unknown=[a, b, c], channel30=channel30,
                           positionChannels=[px, py, pz], rotationChannels=[rx, ry, rz]))
    frames = list(struct.unpack_from("<%df" % value_count, data, end+12)) if value_count else []
    if not all(math.isfinite(value) for value in frames):
        raise FormatError("EZA contains non-finite keyframe values")
    try:
        fps, duration = _f32(1.0/frame_time), _f32(max(0, frame_count-1)*frame_time)
    except OverflowError as exc:
        raise FormatError("EZA timing exceeds the native float range") from exc
    return dict(name=str(name), version="1.0", frameCount=frame_count, channelCount=channels,
                frameTime=frame_time, fps=fps, duration=duration,
                tracks=tracks, frames=frames, byteLength=len(data),
                interpolation="linear-position/shortest-arc-quaternion-slerp",
                rotationOrder="ZYX", scaleAnimated=False)


def _f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def _quat_mul(a, b):
    x, y, z, w = a
    xx, yy, zz, ww = b
    return [_f32(value) for value in (w*xx+x*ww+y*zz-z*yy,
            w*yy-x*zz+y*ww+z*xx,
            w*zz+x*yy-y*xx+z*ww,
            w*ww-x*xx-y*yy-z*zz)]


def euler_quaternion(rotation):
    """Native Euler degrees -> [qx,qy,qz,qw], in qz*qy*qx order."""
    if len(rotation) != 3 or not all(math.isfinite(float(value)) for value in rotation):
        raise FormatError("Invalid animation Euler angles")
    x, y, z = [_f32(float(value)*0.5*_DEGREES_TO_RADIANS) for value in rotation]
    qx = [_f32(math.sin(x)), 0, 0, _f32(math.cos(x))]
    qy = [0, _f32(math.sin(y)), 0, _f32(math.cos(y))]
    qz = [0, 0, _f32(math.sin(z)), _f32(math.cos(z))]
    return [_f32(value) for value in _quat_mul(_quat_mul(qz, qy), qx)]


def slerp(from_quaternion, to_quaternion, alpha):
    """Native 0.001 threshold, sign correction, no added normalization."""
    if not math.isfinite(alpha):
        raise FormatError("Invalid animation interpolation time")
    to = list(to_quaternion)
    cosine = sum(a*b for a, b in zip(from_quaternion, to))
    if cosine < 0:
        cosine = -cosine
        to = [-value for value in to]
    if 1.0-cosine > 0.001:
        omega = math.acos(min(1.0, max(-1.0, cosine)))
        sine = math.sin(omega)
        scale0 = math.sin((1.0-alpha)*omega)/sine
        scale1 = math.sin(alpha*omega)/sine
    else:
        scale0, scale1 = 1.0-alpha, alpha
    return [_f32(scale0*a+scale1*b) for a, b in zip(from_quaternion, to)]


def sample_pose(animation, seconds, bones=None, loop=True):
    """Return {time, frame, nextFrame, alpha, bones:[local pose dictionaries]}.

    Pass the scene object's bind bones to preserve unanimated axes, scaling,
    hierarchy and unrelated bones. Without bind data, tracks receive zero
    position/unit scale. Animated bones gain quaternion=[x,y,z,w]. Existing
    Euler rotation metadata remains a bind value; use quaternion for rendering.
    A track absent from the owner's bones is skipped, matching the game.
    """
    if not math.isfinite(float(seconds)):
        raise FormatError("Animation sample time must be finite")
    if bones is None:
        ids = sorted({int(track["bone"]) for track in animation["tracks"]})
        poses = [dict(id=bone, parent=-1, position=[0, 0, 0], rotation=[0, 0, 0], scale=[1, 1, 1]) for bone in ids]
    else:
        poses = copy.deepcopy(list(bones))
    by_id = {int(bone["id"]): bone for bone in poses}
    count, channels = int(animation["frameCount"]), int(animation["channelCount"])
    if not count:
        return dict(time=0.0, frame=0, nextFrame=0, alpha=0.0, bones=poses)
    duration = float(animation["duration"])
    time = max(0.0, float(seconds))
    if duration > 0:
        time = time % duration if loop else min(time, duration)
    else:
        time = 0.0
    frame_position = time/float(animation["frameTime"])
    frame = min(count-1, max(0, int(math.floor(frame_position))))
    following = min(count-1, frame+1)
    alpha = min(1.0, max(0.0, frame_position-frame)) if following != frame else 0.0
    u = 1.0-alpha
    values = animation["frames"]
    current_start, next_start = channels*frame, channels*following
    for track in animation["tracks"]:
        bone = by_id.get(int(track["bone"]))
        if bone is None:
            continue
        position = list(bone.get("position", [0, 0, 0]))
        for axis, channel in enumerate(track["positionChannels"]):
            if channel != -1:
                position[axis] = _f32(alpha*values[next_start+channel]+u*values[current_start+channel])
        current_angles, next_angles = [], []
        for channel in track["rotationChannels"]:
            current_angles.append(values[current_start+channel] if channel != -1 else 0.0)
            next_angles.append(values[next_start+channel] if channel != -1 else 0.0)
        current_quaternion = euler_quaternion(current_angles)
        next_quaternion = euler_quaternion(next_angles)
        bone["position"] = position
        bone["quaternion"] = slerp(next_quaternion, current_quaternion, u)
    return dict(time=time, frame=frame, nextFrame=following, alpha=alpha, bones=poses)
