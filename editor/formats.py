"""Read-only game archive access and editable, validated source asset formats.

Layouts follow the restored LoadMeat(0x418cd0), ResourceSlot::Load(0x4168f0),
ArchiveStorageProbe::Find(0x401a50), Scene and Bone readers. No game is run.
Original XML nodes/attributes/comments not exposed as fields remain in the tree.
"""
from __future__ import annotations

import copy
import io
import math
import pathlib
import re
import struct
import xml.etree.ElementTree as ET
import zlib


class FormatError(ValueError):
    pass


def asset_key(name):
    return str(name).replace("/", "\\").lower()


class Archive:
    """EZD index; payloads are read on demand, never written or extracted."""
    def __init__(self, path):
        self.path = pathlib.Path(path)
        self.entries = []
        self._by_name = {}
        actual = self.path.stat().st_size
        with self.path.open("rb") as stream:
            header = stream.read(16)
            if len(header) != 16 or header[:4] != b"ezd\xff":
                raise FormatError("Not an EZD archive")
            count, end, size = struct.unpack_from("<III", header, 4)
            if size != actual or not 16 <= end <= size or count > (end-16)//18:
                raise FormatError("Invalid EZD header lengths")
            directory = stream.read(end-16)
        pos = 0
        for i in range(count):
            fields = []
            for _ in range(2):
                nul = directory.find(b"\0", pos)
                if nul < 0:
                    raise FormatError("Unterminated EZD entry name")
                fields.append(directory[pos:nul].decode("cp1251"))
                pos = nul+1
            if pos+16 > len(directory):
                raise FormatError("Truncated EZD directory")
            kind, offset, plain, packed = struct.unpack_from("<IIII", directory, pos)
            pos += 16
            if offset < end or offset+packed > size:
                raise FormatError("EZD payload outside archive")
            if plain > 512*1024*1024:
                raise FormatError("EZD member exceeds editor size limit")
            entry = dict(name=fields[0], secondaryName=fields[1], kind=kind,
                         offset=offset, size=plain, packed=packed, index=i)
            self.entries.append(entry)
            # Native lookup walks records from the end. Later duplicates win.
            self._by_name[asset_key(fields[0])] = entry
        self.directory_end = end

    def names(self):
        return [entry["name"] for entry in self.entries]

    def entry(self, name):
        try:
            return dict(self._by_name[asset_key(name)])
        except KeyError:
            raise FileNotFoundError(str(name)) from None

    def read(self, name):
        entry = self.entry(name)
        with self.path.open("rb") as stream:
            stream.seek(entry["offset"])
            packed = stream.read(entry["packed"])
        if len(packed) != entry["packed"]:
            raise FormatError("Archive changed or payload truncated")
        if entry["kind"] == 2:
            try:
                decoder = zlib.decompressobj()
                data = decoder.decompress(packed, entry["size"]+1)
                if decoder.unconsumed_tail or not decoder.eof or decoder.unused_data:
                    raise FormatError("Invalid or oversized compressed EZD member")
            except zlib.error as exc:
                raise FormatError("Invalid compressed EZD member") from exc
        elif entry["packed"] == entry["size"]:
            data = packed
        else:
            raise FormatError("Unsupported EZD compression kind")
        if len(data) != entry["size"]:
            raise FormatError("EZD decoded size mismatch")
        return data

    def build(self, replacements):
        """Build a mod archive in memory; original file is opened read-only.

        All duplicate records and unchanged compressed payloads stay in their
        original order. Replacements target the native last-wins record only.
        New members are appended. Unused historical directory slack is dropped.
        """
        out = io.BytesIO()
        self.build_to(out, replacements)
        return out.getvalue()

    def build_to(self, stream, replacements):
        pending = {}
        for name, data in replacements.items():
            key = asset_key(name)
            if not key or "\0" in str(name) or len(str(name).encode("cp1251")) > 4096:
                raise FormatError("Invalid archive member name")
            if not isinstance(data, bytes):
                raise FormatError("Archive replacements must be bytes")
            pending[key] = (str(name).replace("/", "\\"), data)
        rows = [dict(entry) for entry in self.entries]
        changed = {}
        for key, (name, data) in pending.items():
            if key in self._by_name:
                index = self._by_name[key]["index"]
            else:
                index = len(rows)
                rows.append(dict(name=name, secondaryName="", kind=2, index=index))
            packed = zlib.compress(data, 9)
            rows[index].update(size=len(data), packed=len(packed), kind=2)
            changed[index] = packed
        directory_size = sum(len(row["name"].encode("cp1251"))+1 +
                             len(row["secondaryName"].encode("cp1251"))+1+16 for row in rows)
        offset = 16+directory_size
        directory = bytearray()
        for row in rows:
            row["newOffset"] = offset
            directory.extend(row["name"].encode("cp1251")+b"\0")
            directory.extend(row["secondaryName"].encode("cp1251")+b"\0")
            directory.extend(struct.pack("<IIII", row["kind"], offset, row["size"], row["packed"]))
            offset += row["packed"]
        if offset >= 2**32:
            raise FormatError("Archive exceeds 32-bit native format")
        stream.write(b"ezd\xff"+struct.pack("<III", len(rows), 16+directory_size, offset))
        stream.write(directory)
        with self.path.open("rb") as original:
            for row in rows:
                if row["index"] in changed:
                    stream.write(changed[row["index"]])
                    continue
                original.seek(row["offset"])
                remaining = row["packed"]
                while remaining:
                    chunk = original.read(min(1024*1024, remaining))
                    if not chunk:
                        raise FormatError("Original archive payload truncated")
                    stream.write(chunk)
                    remaining -= len(chunk)
        return offset


class _Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, n):
        if n < 0 or self.pos+n > len(self.data):
            raise FormatError("Truncated binary asset at byte %d" % self.pos)
        data = self.data[self.pos:self.pos+n]
        self.pos += n
        return data

    def number(self, fmt="I"):
        return struct.unpack("<"+fmt, self.take(struct.calcsize("<"+fmt)))[0]

    def array(self, count, fmt):
        if count < 0 or count > 16_000_000:
            raise FormatError("Invalid asset array count")
        size = struct.calcsize("<"+fmt)
        return list(struct.unpack("<"+str(count)+fmt, self.take(count*size))) if count else []

    def finish(self):
        if self.pos != len(self.data):
            raise FormatError("Unexpected trailing asset data: %d bytes" % (len(self.data)-self.pos))


def _text32(raw):
    return raw.split(b"\0", 1)[0].decode("cp1251")


def parse_meat(data, name="mesh.meat"):
    r = _Reader(data)
    if r.take(4) != b"MEAT":
        raise FormatError("Not a MEAT mesh")
    version = r.number()
    if version not in (20, 21):
        raise FormatError("Unsupported MEAT version %d (supported: 20, 21)" % version)
    slots, material_count = r.number(), r.number()
    if slots > 1_000_000 or material_count > 100_000:
        raise FormatError("Invalid MEAT slot/material count")
    material_raw = [r.take(32) for _ in range(material_count)]
    materials = [_text32(raw) for raw in material_raw]
    count = r.number()
    if count > 100_000:
        raise FormatError("Invalid MEAT mesh count")
    base = pathlib.PureWindowsPath(name).stem.lower()
    result = dict(version=version, slotCount=slots, partCount=slots,
                  materialNames=materials, materialRaw=[raw.hex() for raw in material_raw], parts=[])
    for part_id in range(count):
        vertices = r.number()
        positions = r.array(vertices*3, "f")
        normals = r.array(vertices*3, "f") if version == 21 else []
        triangles = r.number()
        indices = r.array(triangles*3, "I")
        if any(index >= vertices for index in indices):
            raise FormatError("MEAT triangle references missing vertex")
        flags = r.number("B")
        if flags & ~15:
            raise FormatError("Unknown MEAT map flags")
        face_materials = r.array(triangles, "I") if flags else []
        if any(index >= material_count for index in face_materials):
            raise FormatError("MEAT triangle references missing material")
        uv = r.array(triangles*6, "f") if flags & 1 else []
        uv2 = r.array(triangles*6, "f") if flags & 2 else []
        uv3 = r.array(triangles*6, "f") if flags & 4 else []
        bones = r.array(vertices, "i") if slots else [0]*vertices
        # Some archived models use IDs outside slotCount; preserve IDs verbatim.
        numeric = positions+normals+uv+uv2+uv3
        if not all(math.isfinite(value) for value in numeric):
            raise FormatError("MEAT contains non-finite geometry")
        result["parts"].append(dict(name=base+(str(part_id) if part_id else ""),
            positions=positions, normals=normals, indices=indices, flags=flags,
            slotCount=slots, partCount=slots or 1, vertexBones=bones,
            uv=uv, uv2=uv2, uv3=uv3, materialNames=list(materials), faceMaterials=face_materials))
    r.finish()
    result["byteLength"] = len(data)
    return result


def _finite(values, expected, label):
    if len(values) != expected or not all(isinstance(v, (int, float)) and math.isfinite(v) for v in values):
        raise FormatError("Invalid "+label)
    return values


def write_meat(mesh):
    version = int(mesh.get("version", 20))
    if version not in (20, 21):
        raise FormatError("MEAT writer supports versions 20 and 21")
    parts = mesh["parts"]
    materials = mesh.get("materialNames", parts[0].get("materialNames", []) if parts else [])
    slots = int(mesh.get("slotCount", mesh.get("partCount", 0)))
    if slots < 0 or slots > 1_000_000:
        raise FormatError("Invalid MEAT slot count")
    out = bytearray(b"MEAT"+struct.pack("<III", version, slots, len(materials)))
    old_raw = mesh.get("materialRaw", [])
    for i, name in enumerate(materials):
        encoded = str(name).encode("cp1251")
        if b"\0" in encoded or len(encoded) > 31:
            raise FormatError("MEAT material names must fit 31 CP1251 bytes")
        raw = bytes.fromhex(old_raw[i]) if i < len(old_raw) else b""
        out.extend(raw if len(raw) == 32 and _text32(raw) == name else encoded.ljust(32, b"\0"))
    out.extend(struct.pack("<I", len(parts)))
    for part in parts:
        positions, indices = part["positions"], part["indices"]
        if len(positions) % 3 or len(indices) % 3:
            raise FormatError("MEAT positions and indices must be triplets")
        vertices, faces = len(positions)//3, len(indices)//3
        _finite(positions, vertices*3, "vertex positions")
        if any(not isinstance(i, int) or not 0 <= i < vertices for i in indices):
            raise FormatError("Invalid MEAT triangle index")
        flags = int(part.get("flags", (1 if part.get("uv") else 0) | (2 if part.get("uv2") else 0) | (4 if part.get("uv3") else 0)))
        if flags & ~15:
            raise FormatError("Unknown MEAT map flags")
        out.extend(struct.pack("<I", vertices))
        out.extend(struct.pack("<%df" % len(positions), *positions))
        if version == 21:
            normals = _finite(part.get("normals", []), vertices*3, "normals")
            out.extend(struct.pack("<%df" % len(normals), *normals))
        out.extend(struct.pack("<I", faces))
        out.extend(struct.pack("<%dI" % len(indices), *indices))
        out.append(flags)
        if flags:
            ids = part.get("faceMaterials", [0]*faces)
            if len(ids) != faces or any(not isinstance(i, int) or not 0 <= i < len(materials) for i in ids):
                raise FormatError("Invalid MEAT face material index")
            out.extend(struct.pack("<%dI" % faces, *ids))
        for bit, key in ((1, "uv"), (2, "uv2"), (4, "uv3")):
            if flags & bit:
                values = _finite(part.get(key, []), faces*6, key)
                out.extend(struct.pack("<%df" % len(values), *values))
        if slots:
            bones = part.get("vertexBones", [0]*vertices)
            if len(bones) != vertices or any(not isinstance(i, int) or not -2**31 <= i < 2**31 for i in bones):
                raise FormatError("Invalid MEAT bone IDs")
            out.extend(struct.pack("<%di" % vertices, *bones))
    encoded = bytes(out)
    parse_meat(encoded)
    return encoded


def parse_mat(data, name=""):
    r = _Reader(data)
    if r.take(3) != b"MAT":
        raise FormatError("Not a MAT material")
    version = r.number()
    if version < 1:
        raise FormatError("Invalid MAT version")
    result = dict(version=version)
    for key in ("ambient", "diffuse", "specular"):
        color = r.number()
        result[key] = [(color >> shift & 255)/255 for shift in (0, 8, 16)]
        result[key+"Packed"] = color
    for key in ("shininess", "value54", "value58"):
        result[key] = r.number("f")
    flags = r.number("B")
    if flags & ~15:
        raise FormatError("Unknown MAT map flags")
    result.update(flags=flags, blend=False, twoSided=False)
    if version > 1:
        result["blend"] = r.number("B") == 1
        result["twoSided"] = r.number("B") == 1
    directory = str(pathlib.PureWindowsPath(name).parent)
    directory = "" if directory == "." else directory+"\\"
    textures, texture_names, maps = [], [], []
    for bit, kind in ((1, "diffuse"), (2, "layer2"), (4, "layer3"), (8, "environment")):
        if flags & bit:
            texture = _text32(r.take(32))
            texture_names.append(texture)
            textures.append(directory+texture)
            maps.append(dict(kind=kind, filename=directory+texture, name=texture))
    result.update(textures=textures, textureNames=texture_names, maps=maps)
    r.finish()
    return result


_FLOAT_PREFIX = re.compile(r"^[\s]*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)")


def _atof(value, default=0):
    # The original C atof accepts the prefix in malformed '-5.70.2114'.
    m = _FLOAT_PREFIX.match(str(value))
    result = float(m.group(1)) if m else float(default)
    return result if math.isfinite(result) else float(default)


def _int(value, default=0):
    return int(_atof(value, default))


def _decode_xml(data):
    if isinstance(data, bytes):
        if data.startswith(b"\xef\xbb\xbf") or re.match(br"\s*<\?xml[^>]*encoding=[\"']utf-?8[\"']", data, re.I):
            text = data.decode("utf-8-sig")
        else:
            # Several original maps contain byte 0x98 in object metadata. It is
            # undefined in CP1251; retain it as U+0098 and restore the raw byte
            # when saving, rather than replacing/deleting original metadata.
            text = data.decode("cp1251", "surrogateescape").replace("\udc98", "\x98")
    else:
        text = str(data)
    return text


def _xml_root(data):
    text = _decode_xml(data)
    if re.search(r"<!\s*(?:DOCTYPE|ENTITY)\b", text, re.I):
        raise FormatError("XML external entities/DTDs are not supported")
    # XML text is already decoded. Drop a legacy encoding declaration before ET.
    text = re.sub(r"^\s*<\?xml[^?]*\?>", "", text)
    try:
        return ET.fromstring(text, parser=ET.XMLParser(target=ET.TreeBuilder(insert_comments=True)))
    except ET.ParseError as exc:
        raise FormatError("Invalid scene XML: "+str(exc)) from exc


def _xml_string(node):
    return ET.tostring(node, encoding="unicode")


def _vec(node, key, default=(0, 0, 0)):
    element = node.find(key)
    return [_atof(element.get(k, default[i]), default[i]) for i, k in enumerate("xyz")] if element is not None else list(default)


def _put_vec(node, key, values):
    _finite(values, 3, key)
    element = node.find(key)
    if element is None:
        element = ET.SubElement(node, key)
    for key, value in zip("xyz", values):
        # Keep odd original spellings when numeric value is unchanged.
        if key not in element.attrib or _atof(element.get(key)) != float(value):
            element.set(key, format(float(value), ".9g"))


def parse_scene(data, name="scene.xml"):
    root = _xml_root(data)
    if root.tag != "scene":
        raise FormatError("XML root is not scene")
    camera = root.find("camera")
    camera_data = dict(fov=_atof(camera.get("fov", 45), 45), position=_vec(camera, "position"), rotation=_vec(camera, "rotation"), present=True) if camera is not None else dict(fov=45, position=[0, 0, 0], rotation=[0, 0, 0], present=False)
    objects = []
    for group in ("static_objects", "dynamic_objects", "interface_objects"):
        section = root.find(group)
        if section is None:
            continue
        for index, node in enumerate(section.findall("object")):
            position = node.find("position")
            bones = []
            for bone in node.findall("./position/bones/bone"):
                bones.append(dict(id=_int(bone.get("bone_id", len(bones))),
                    parent=_int(bone.get("parent_bone_id", -1), -1), name=bone.get("name", ""),
                    position=_vec(bone, "position"), rotation=_vec(bone, "rotation"),
                    scale=_vec(bone, "scale", (1, 1, 1)), center=_vec(bone, "center"), bbox=_vec(bone, "bbox"),
                    radius=_atof(bone.get("radius", 0)), attributes=dict(bone.attrib)))
            lods = [dict(child.attrib, type=child.tag) for child in node if child.tag in ("lod", "srb_lod")]
            objects.append(dict(id=group+":"+str(index), name=node.get("name", ""), group=group,
                attributes=dict(node.attrib), bones=bones, lods=lods,
                ownerBone=_int(position.get("owner_bone_id", 0)) if position is not None else 0,
                positionAttributes=dict(position.attrib) if position is not None else {},
                components=[child.tag for child in node if isinstance(child.tag, str)], xml=_xml_string(node)))
    return dict(name=name, camera=camera_data, objects=objects, xml=_xml_string(root))


def apply_scene(original, scene):
    """Apply editable fields to a copy; unknown components and comments survive.

    Object list is authoritative (deletion/reordering/duplication). For a new ID,
    its xml is the template. IDs are editor identities, never written into game XML.
    """
    root = _xml_root(original)
    if root.tag != "scene":
        raise FormatError("XML root is not scene")
    known = {}
    sections = {}
    for group in ("static_objects", "dynamic_objects", "interface_objects"):
        section = root.find(group)
        if section is not None:
            sections[group] = section
            for index, node in enumerate(section.findall("object")):
                known[group+":"+str(index)] = node
            for node in list(section):
                if node.tag == "object":
                    section.remove(node)
    for obj in scene.get("objects", []):
        group = obj.get("group", "static_objects")
        if group not in ("static_objects", "dynamic_objects", "interface_objects"):
            raise FormatError("Invalid scene object group")
        node = _xml_root(obj["xml"]) if obj.get("xml") else copy.deepcopy(known.get(obj.get("id"), ET.Element("object")))
        if node.tag != "object":
            raise FormatError("Object template root must be object")
        if "attributes" in obj:
            node.attrib.clear()
            node.attrib.update({str(k): str(v) for k, v in obj["attributes"].items()})
        if "name" in obj:
            node.set("name", str(obj["name"]))
        if "bones" in obj and (obj["bones"] or node.find("position") is not None):
            bone_list = obj["bones"]
            bone_ids = [int(bone["id"]) for bone in bone_list]
            if bone_ids != list(range(len(bone_list))):
                raise FormatError("Game bone IDs must be contiguous and ordered from zero")
            for bone in bone_list:
                parent = int(bone.get("parent", -1))
                if parent != -1 and not 0 <= parent < int(bone["id"]):
                    raise FormatError("Bone parent must precede child (no cycles or dangling parents)")
            position = node.find("position")
            if position is None:
                position = ET.SubElement(node, "position")
            if "positionAttributes" in obj:
                position.attrib.update({str(k): str(v) for k, v in obj["positionAttributes"].items()})
            position.set("bonecount", str(len(obj["bones"])))
            position.set("owner_bone_id", str(int(obj.get("ownerBone", position.get("owner_bone_id", 0)))))
            if bone_ids and int(position.get("owner_bone_id")) not in bone_ids:
                raise FormatError("Object owner bone is missing")
            bones_node = position.find("bones")
            if bones_node is None:
                bones_node = ET.SubElement(position, "bones")
            old = {str(b.get("bone_id")): b for b in bones_node.findall("bone")}
            for bone in list(bones_node):
                if bone.tag == "bone":
                    bones_node.remove(bone)
            ids = set()
            for bone in obj["bones"]:
                bone_id = int(bone["id"])
                if bone_id in ids:
                    raise FormatError("Duplicate bone ID")
                ids.add(bone_id)
                entry = copy.deepcopy(old[str(bone_id)]) if str(bone_id) in old else ET.Element("bone")
                if "attributes" in bone:
                    entry.attrib.update({str(k): str(v) for k, v in bone["attributes"].items()})
                entry.set("bone_id", str(bone_id))
                if int(bone.get("parent", -1)) != -1 or "parent_bone_id" in entry.attrib:
                    entry.set("parent_bone_id", str(int(bone.get("parent", -1))))
                if bone.get("name") or "name" in entry.attrib:
                    entry.set("name", str(bone.get("name", "")))
                if "radius" in entry.attrib or bone.get("radius", 0) != 0:
                    _finite([bone.get("radius", 0)], 1, "bone radius")
                    entry.set("radius", format(float(bone.get("radius", 0)), ".9g"))
                for key in ("position", "rotation", "scale", "center", "bbox"):
                    if key in bone and (entry.find(key) is not None or bone[key] != ([1, 1, 1] if key == "scale" else [0, 0, 0])):
                        _put_vec(entry, key, bone[key])
                bones_node.append(entry)
        if "lods" in obj:
            old_lods = [child for child in node if child.tag in ("lod", "srb_lod")]
            new_lods = []
            for index, lod in enumerate(obj["lods"]):
                entry = copy.deepcopy(old_lods[index]) if index < len(old_lods) else ET.Element("lod")
                kind = lod.get("type", entry.tag)
                if kind not in ("lod", "srb_lod"):
                    raise FormatError("Invalid LOD component type")
                entry.tag = kind
                entry.attrib.update({str(k): str(v) for k, v in lod.items() if k != "type"})
                new_lods.append(entry)
            # Component traversal order is meaningful in the game. Replace at
            # the old location rather than moving every LOD after all scripts.
            iterator = iter(new_lods)
            for child in list(node):
                if child.tag in ("lod", "srb_lod"):
                    index = list(node).index(child)
                    node.remove(child)
                    replacement = next(iterator, None)
                    if replacement is not None:
                        node.insert(index, replacement)
            for replacement in iterator:
                node.append(replacement)
        section = sections.get(group)
        if section is None:
            section = ET.SubElement(root, group)
            sections[group] = section
        section.append(node)
    if "camera" in scene and (root.find("camera") is not None or scene["camera"].get("present", True)):
        camera = root.find("camera")
        if camera is None:
            camera = ET.SubElement(root, "camera")
        data = scene["camera"]
        if "fov" in data:
            value = float(data["fov"])
            if not math.isfinite(value) or not 0 < value < 180:
                raise FormatError("Camera FOV must be between 0 and 180")
            if _atof(camera.get("fov")) != value:
                camera.set("fov", format(value, ".9g"))
        for key in ("position", "rotation"):
            if key in data:
                _put_vec(camera, key, data[key])
    source_text = _decode_xml(original)
    start = re.search(r"<scene(?:\s|>)", source_text)
    end = source_text.rfind("</scene>")
    prefix = source_text[:start.start()] if start else ""
    prefix = re.sub(r"^\s*<\?xml[^?]*\?>", "", prefix)
    suffix = source_text[end+8:] if end >= 0 else ""
    text = '<?xml version="1.0" encoding="windows-1251"?>\n'+prefix+_xml_string(root)+suffix
    encoded = text.encode("cp1251", "xmlcharrefreplace").replace(b"&#152;", b"\x98")
    parse_scene(encoded, scene.get("name", "scene.xml"))
    return encoded
