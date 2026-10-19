"""Interchange and native mod export. No writes to the source game archive."""
import copy
import io
import json
import math
import pathlib
import re
import struct
import base64
import zipfile

from PIL import Image
from formats import parse_meat, parse_mat, write_meat, apply_scene


def identity():
    return [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]


def mul(a, b):
    return [sum(a[k*4+r]*b[c*4+k] for k in range(4)) for c in range(4) for r in range(4)]


def rotation(angles):
    x, y, z = (math.radians(float(v)) for v in angles)
    cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
    return [cz*cy, sz*cy, -sy, 0., cz*sy*sx-sz*cx, sz*sy*sx+cz*cx, cy*sx, 0.,
            cz*sy*cx+sz*sx, sz*sy*cx-cz*sx, cy*cx, 0., 0., 0., 0., 1.]


def matrix(transform, original=False):
    m = rotation(transform.get('rotation', [0, 0, 0]))
    scale = transform.get('scale', [1, 1, 1])
    # Original bone is T*S*R; editor delta is ordinary T*R*S.
    for col in range(3):
        for row in range(3):
            m[col*4+row] *= float(scale[row if original else col])
    m[12:15] = [float(v) for v in transform.get('position', [0, 0, 0])]
    return m


def transform_point(m, p):
    return [sum(m[k*4+r]*p[k] for k in range(3))+m[12+r] for r in range(3)]


def bone_world(obj):
    bones = {str(b.get('id', i)): b for i, b in enumerate(obj.get('bones', []))}
    done, active = {}, set()
    def visit(key):
        key = str(key)
        if key in done:
            return done[key]
        if key not in bones:
            return identity()
        if key in active:
            raise ValueError('Cyclic bone hierarchy')
        active.add(key)
        b = bones[key]
        local = matrix(b, original=True)
        parent = b.get('parent', -1)
        if str(parent) in bones and str(parent) != key:
            local = mul(visit(parent), local)
        active.remove(key)
        done[key] = local
        return local
    for key in bones:
        visit(key)
    return done


def safe_name(value):
    return re.sub(r'[^a-zA-Z0-9_-]', '_', str(value))[:60] or 'object'


def texture_png(archive, name):
    with Image.open(io.BytesIO(archive.read(name))) as im:
        result = io.BytesIO()
        im.convert('RGBA').save(result, 'PNG')
        return result.getvalue()


def object_meshes(archive, obj):
    if obj.get('editorMesh'):
        yield obj['editorMesh']
        return
    # Only nearest LOD is shown/exported, not multiple overlapping LOD copies.
    lods = obj.get('lods', [])
    if lods:
        name = lods[0]['meatname'].replace(' ', '')
        yield parse_meat(archive.read(name), name)


def obj_zip(archive, scene, selected=None):
    out = io.BytesIO()
    lines = ['# Titbit Studio, Z up, scene coordinates', 'mtllib materials.mtl']
    materials, files = {}, {}
    vertex_offset = uv_offset = 0
    objects = [o for o in scene['objects'] if not selected or o['id'] == selected]
    if selected and not objects:
        raise ValueError('Object not found')
    for obj in objects:
        delta, bones = matrix(obj.get('editorTransform', {})), bone_world(obj)
        custom = bool(obj.get('editorMesh'))
        imported = bool(obj.get('editorImported'))
        for mesh in object_meshes(archive, obj):
            for part in mesh['parts']:
                positions, indices = part['positions'], part['indices']
                if not indices:
                    continue
                lines.append('o '+safe_name(obj['name'])+'_'+safe_name(part.get('name', 'mesh')))
                v_bones = part.get('vertexBones', [])
                for v in range(len(positions)//3):
                    p = positions[v*3:v*3+3]
                    if not imported:
                        p = transform_point(bones.get(str(v_bones[v] if v < len(v_bones) else 0), identity()), p)
                    p = transform_point(delta, p)
                    lines.append('v '+' '.join(format(x, '.9g') for x in p))
                uvs = part.get('uv', [])
                vertex_uv = bool(part.get('uvLayout') == 'vertex')
                if uvs and len(uvs) != len(indices)*2 and len(uvs) == len(positions)//3*2:
                    vertex_uv = True
                if vertex_uv:
                    uvs = [uvs[v*2+k] for v in indices for k in range(2)]
                for i in range(0, len(uvs), 2):
                    lines.append('vt '+format(uvs[i], '.9g')+' '+format(uvs[i+1], '.9g'))
                names = part.get('materialNames', mesh.get('materialNames', []))
                face_mat = part.get('faceMaterials', [])
                previous = None
                for face in range(len(indices)//3):
                    matid = face_mat[face] if face < len(face_mat) else 0
                    original_name = names[matid] if 0 <= matid < len(names) else 'default'
                    matname = safe_name(original_name)
                    if matname not in materials:
                        material = None
                        if custom:
                            material = next((m for m in mesh.get('materials', []) if m.get('name') == original_name), None)
                        if not custom and names:
                            try:
                                filename = original_name if original_name.lower().endswith('.mat') else original_name+'.mat'
                                material = parse_mat(archive.read(filename), filename)
                            except (KeyError, FileNotFoundError, ValueError):
                                material = None
                        block = ['newmtl '+matname, 'Kd 1 1 1', 'Ka 0 0 0', 'd 1', 'illum 1']
                        if material and material.get('color'):
                            block[1] = 'Kd '+' '.join(str(c) for c in material['color'][:3])
                        textures = material.get('textures', []) if material else []
                        if textures:
                            texture = textures[0]
                            texpath = 'textures/'+safe_name(pathlib.Path(texture).stem)+'.png'
                            files[texpath] = texture_png(archive, texture)
                            block.append('map_Kd '+texpath)
                        elif material and material.get('texture', '').startswith('data:image/'):
                            texpath = 'textures/'+matname+'.png'
                            data = base64.b64decode(material['texture'].split(',', 1)[1], validate=True)
                            with Image.open(io.BytesIO(data)) as image:
                                if material.get('flipY') is False:
                                    image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                                encoded = io.BytesIO(); image.convert('RGBA').save(encoded, 'PNG')
                            files[texpath] = encoded.getvalue()
                            block.append('map_Kd '+texpath)
                        materials[matname] = '\n'.join(block)+'\n'
                    if matname != previous:
                        lines.append('usemtl '+matname); previous = matname
                    refs = []
                    for k in range(3):
                        refs.append(str(indices[face*3+k]+1+vertex_offset)+
                                    ('/'+str(uv_offset+face*3+k+1) if uvs else ''))
                    lines.append('f '+' '.join(refs))
                vertex_offset += len(positions)//3
                uv_offset += len(uvs)//2
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('scene.obj', '\n'.join(lines)+'\n')
        z.writestr('materials.mtl', '\n'.join(materials.values()))
        for name, data in files.items():
            z.writestr(name, data)
    return out.getvalue()


def decompose_bone(m):
    """Recover T*S*R, rejecting shear rather than silently corrupting a mod."""
    rows = [[m[c*4+r] for c in range(3)] for r in range(3)]
    scales = [math.sqrt(sum(x*x for x in row)) for row in rows]
    if min(scales) < 1e-8:
        raise ValueError('Нулевой масштаб нельзя экспортировать в игровой скелет')
    r = [[v/scales[i] for v in row] for i, row in enumerate(rows)]
    det = (r[0][0]*(r[1][1]*r[2][2]-r[1][2]*r[2][1])-
           r[0][1]*(r[1][0]*r[2][2]-r[1][2]*r[2][0])+r[0][2]*(r[1][0]*r[2][1]-r[1][1]*r[2][0]))
    if det < 0:
        scales[0] *= -1; r[0] = [-v for v in r[0]]
    if any(abs(sum(r[i][k]*r[j][k] for k in range(3))) > 0.00005 for i in range(3) for j in range(i)):
        raise ValueError('Игровой формат не хранит сдвиг осей (shear). Используйте равномерный масштаб или экспорт GLB.')
    y = math.asin(max(-1., min(1., -r[2][0])))
    if abs(math.cos(y)) > 1e-7:
        x, z = math.atan2(r[2][1], r[2][2]), math.atan2(r[1][0], r[0][0])
    else:
        x, z = math.atan2(-r[1][2], r[1][1]), 0.
    return {'position': m[12:15], 'rotation': [math.degrees(v) for v in (x, y, z)], 'scale': scales}


def native_replacements(archive, scene):
    result, model = {}, copy.deepcopy(scene)
    for index, obj in enumerate(model['objects']):
        delta = matrix(obj.get('editorTransform', {}))
        if obj.get('editorMesh'):
            mesh = copy.deepcopy(obj['editorMesh'])
            material_names = []
            material_map = {}
            for part in mesh['parts']:
                names = part.get('materialNames', ['default'])
                mapped = []
                for name in names:
                    if name not in material_map:
                        material_map[name] = len(material_names)
                        newname = f'ed_{index}_{len(material_names)}'
                        material_names.append(newname)
                        material = next((m for m in mesh.get('materials', []) if m.get('name') == name), {})
                        texture = material.get('texture', '')
                        texture_name = ''
                        if texture.startswith('data:image/'):
                            raw = base64.b64decode(texture.split(',', 1)[1], validate=True)
                            with Image.open(io.BytesIO(raw)) as image:
                                if material.get('flipY') is False:
                                    image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                                buffer = io.BytesIO(); image.convert('RGBA').save(buffer, 'TGA')
                            texture_name = f'ed_{index}_{len(material_names)-1}.tga'
                            result[texture_name] = buffer.getvalue()
                        else:
                            texture_name = f'ed_{index}_{len(material_names)-1}.tga'
                            buffer = io.BytesIO()
                            rgb = tuple(max(0, min(255, round(c*255))) for c in material.get('color', [1, 1, 1])[:3])
                            Image.new('RGB', (1, 1), rgb).save(buffer, 'TGA')
                            result[texture_name] = buffer.getvalue()
                        color = [max(0, min(255, round(c*255))) for c in material.get('color', [1, 1, 1])[:3]]
                        packed_color = color[0] | color[1]<<8 | color[2]<<16
                        mat = b'MAT'+struct.pack('<IIIIfffBBB', 2, 0, packed_color, 0, 0.25, .05, 1., bool(texture_name), 1, 1)
                        if texture_name:
                            mat += texture_name.encode('ascii').ljust(32, b'\0')
                        result[newname+'.mat'] = mat
                    mapped.append(material_map[name])
                faces = part.get('faceMaterials', [0]*(len(part['indices'])//3))
                part['faceMaterials'] = [mapped[i] for i in faces]
                uv = part.get('uv', [])
                if part.get('uvLayout') == 'vertex' or (uv and len(uv) != len(part['indices'])*2 and len(uv) == len(part['positions'])//3*2):
                    part['uv'] = [uv[v*2+k] for v in part['indices'] for k in range(2)]
                if not part.get('uv'):
                    part['uv'] = [0.]*(len(part['indices'])*2)
                part['flags'] = 1; part['vertexBones'] = [0]*(len(part['positions'])//3)
                part['materialNames'] = material_names
            mesh['version'] = 20; mesh['slotCount'] = 1; mesh['materialNames'] = material_names
            # Materials are a global table shared by all parts in this format.
            for part in mesh['parts']:
                part['materialNames'] = material_names
            filename = f'editor_{index}_{safe_name(obj["name"])[:16]}.meat'
            result[filename] = write_meat(mesh)
            replacing = bool(obj.get('editorReplace'))
            if replacing:
                if len(obj.get('bones', [])) != 1:
                    raise ValueError('Замена модели поддерживает только один исходный корневой bone')
                obj['bones'][0].update(decompose_bone(mul(delta, matrix(obj['bones'][0], original=True))))
            else:
                obj['bones'] = [{'id': 0, 'parent': -1, 'name': 'Root', **decompose_bone(delta)}]
            obj['lods'] = [{'meatname': filename, 'neardistance': '0', 'fardistance': '10000'}]
            if not replacing:
                obj['group'] = 'dynamic_objects'
                obj['attributes'] = {'name': obj['name'], 'selectable': '1'}
                obj['xml'] = ''
        else:
            if obj.get('group') not in ('static_objects', 'dynamic_objects', 'interface_objects'):
                obj['group'] = 'dynamic_objects'
            bones = obj.get('bones', [])
            ids = {str(b.get('id', i)) for i, b in enumerate(bones)}
            for b in bones:
                if str(b.get('parent', -1)) not in ids:
                    b.update(decompose_bone(mul(delta, matrix(b, original=True))))
        obj.pop('editorTransform', None)
    try:
        original = archive.read(scene['name'])
    except (KeyError, FileNotFoundError):
        original = scene['xml']
    result[model['name']] = apply_scene(original, model)
    return result


def bundle_zip(archive, scene):
    replacements = native_replacements(archive, scene)
    out = io.BytesIO()
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_STORED) as z:
        z.writestr('QUEST.ezd', archive.build(replacements))
        z.writestr('project.json', json.dumps({'version': 1, 'scenes': {scene['name']: scene}}, ensure_ascii=False, allow_nan=False, indent=2))
        for name, data in replacements.items():
            z.writestr('changed/'+name, data)
        z.writestr('README.txt', ('Titbit Studio: отдельный архив мода.\n'
            'Скопируйте папку modern в новую папку для тестирования.\n'
            'Замените QUEST.ezd только в этой копии. Сохраните оригинал отдельно.\n'
            'Объекты и скрипты сцены сохраняются; анимация импортированных моделей не поддерживается.\n'
            'Игра автоматически не запускается.\n').encode('utf-8-sig'))
    return out.getvalue()
