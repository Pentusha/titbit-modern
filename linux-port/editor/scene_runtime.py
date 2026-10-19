"""Resolve authored scene preview animation references without running scripts."""
import re
import pathlib
import xml.etree.ElementTree as ET

from animation import parse_eza


def strip_comments(text):
    return re.sub(r'//[^\r\n]*|/\*.*?\*/', '', text, flags=re.S)


def on_create(text):
    match = re.search(r'\bvoid\s+ON_CREATE\s*\([^)]*\)\s*\{', text)
    if not match:
        return ''
    start, level = match.end(), 1
    quoted = False
    escape = False
    for i in range(start, len(text)):
        ch = text[i]
        if escape:
            escape = False
        elif ch == '\\' and quoted:
            escape = True
        elif ch == '"':
            quoted = not quoted
        elif not quoted:
            if ch == '{': level += 1
            if ch == '}': level -= 1
            if level == 0: return text[start:i]
    return ''


def scene_animations(archive, scene):
    objects = []
    for obj in scene['objects']:
        clips = {}
        def add(name, initial=False, source='XML', loop=True):
            name = name.replace(' ', '')
            try:
                entry = archive.entry(name)
            except FileNotFoundError:
                return
            key = entry['name'].lower()
            if key in clips:
                if initial:
                    clips[key].update(initial=True, source=source, loop=loop)
                return
            clips[key] = {'name': entry['name'], 'initial': initial, 'source': source, 'loop': loop}
        if obj.get('xml'):
            node = ET.fromstring(obj['xml'])
            for animation in node.findall('./position/animations/skeleton_animation'):
                name = animation.get('animation_name', '')
                if name: add(name, True)
        script = obj.get('attributes', {}).get('scriptname')
        if script:
            try:
                text = strip_comments(archive.read(script).decode('cp1251', errors='replace'))
                initial_body = on_create(text)
                # This is authored-reference discovery, not a SeeR interpreter.
                refs = r'(?:self\s*->\s*)CreateSkeletonAnimation\s*\(\s*"([^"]+)"\s*,\s*(-?\d+)'
                first = re.search(refs, initial_body)
                for match in re.finditer(refs, text):
                    add(match[1], False, script, bool(int(match[2]) & 1))
                if first:
                    add(first[1], True, 'ON_CREATE: '+script, bool(int(first[2]) & 1))
            except FileNotFoundError:
                pass
        # Also expose the model's own authored clips (e.g. bomj_talk), even
        # when another object's script sends its dialogue/animation events.
        for lod in obj.get('lods', []):
            stem = pathlib.PureWindowsPath(lod.get('meatname', '').replace(' ', '')).stem.lower()
            if stem:
                for asset in archive.names():
                    if asset.lower().startswith(stem+'_') and asset.lower().endswith('.eza'):
                        add(asset, False, 'Библиотека модели '+stem)
        if clips:
            # Explicit XML and initial self animation precede alternative clips.
            ordered = sorted(clips.values(), key=lambda c: not c['initial'])
            objects.append({'objectId': obj['id'], 'objectName': obj['name'], 'clips': ordered})
    return {'objects': objects, 'mode': 'authored-animation-preview',
            'notice': 'Анимации из XML и скриптов; квестовые условия и диалоговая VM не исполняются.'}
