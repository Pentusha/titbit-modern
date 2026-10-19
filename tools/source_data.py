"""Expose compiler-proved fields as section-backed COFF data labels.

The locator and sizeof constants are emitted by actual C/C++ source. No image
bytes, absolute external symbols, extra storage or pointer-wrapper variables
are substituted for the field. Existing section payloads/relocations stay intact.
"""
import struct

from coff_link import CoffObject


def scalar(coff, name):
    symbol = coff.by_name[name]
    section = coff.sections[symbol['section']]
    at = symbol['value']
    if (at + 4 > section['size'] or section['flags'] & 0x20
            or any(at <= r['offset'] < at + 4 for r in section['relocations'])):
        raise ValueError('Not a compiler sizeof constant: ' + name)
    return struct.unpack_from('<I', section['bytes'], at)[0]


def append_label(coff, name, section, value, symbol_type=0):
    existing = coff.by_name.get(name)
    if existing:
        if (existing['section'], existing['value'], existing['storage'], existing['type']) == (section, value, 2, symbol_type):
            return coff
        raise ValueError('Conflicting field label: ' + name)
    payload = bytearray(coff.data)
    offset, count = struct.unpack_from('<II', payload, 8)
    string_at = offset + 18 * count
    size = struct.unpack_from('<I', payload, string_at)[0]
    if string_at + size != len(payload):
        raise ValueError('Unexpected trailing COFF payload')
    strings = bytearray(payload[string_at:])
    raw = name.encode('ascii')
    if len(raw) <= 8:
        field = raw.ljust(8, b'\0')
    else:
        field = struct.pack('<II', 0, len(strings))
        strings.extend(raw + b'\0')
    struct.pack_into('<I', strings, 0, len(strings))
    record = struct.pack('<8sIhHBB', field, value, section, symbol_type, 2, 0)
    result = payload[:string_at] + record + strings
    struct.pack_into('<I', result, 12, count + 1)
    revised = CoffObject(coff.path, data=result)
    for index, old in coff.sections.items():
        new = revised.sections[index]
        describe = lambda s: (s['bytes'], s['flags'], s['size'],
                             [(r['offset'], r['kind'], r['symbol']['index']) for r in s['relocations']])
        if describe(old) != describe(new):
            raise ValueError('Field label changed a compiler section')
    return revised


def expose_field(owner, locator, name, spec, native_owner, native_field):
    own = owner.by_name[spec['owner']]
    if own['section'] <= 0 or owner.sections[own['section']]['flags'] & 0x20:
        raise ValueError('Field owner must be real source data, not code/COMMON/absolute')
    symbol = locator.by_name[spec['locator']]
    section = locator.sections[symbol['section']]
    relocations = [r for r in section['relocations'] if r['offset'] == symbol['value']]
    if len(relocations) != 1 or relocations[0]['kind'] != 6:
        raise ValueError('Field locator must be one source pointer relocation')
    relocation = relocations[0]
    target = relocation['symbol']
    addend = struct.unpack_from('<I', section['bytes'], symbol['value'])[0]
    if owner.path == locator.path:
        if target['section'] != own['section']:
            raise ValueError('Local field locator points outside its owner section')
        offset = target['value'] + addend - own['value']
    else:
        if target['name'] != spec['owner'] or target['section'] != 0 or target['value'] != 0:
            raise ValueError('External field locator names a different owner')
        offset = addend
    extent = scalar(locator, spec['extent'])
    owner_extent = scalar(locator, spec['ownerExtent'])
    if offset < 0 or not extent or offset + extent > owner_extent:
        raise ValueError('Field exceeds compiler sizeof owner')
    if own['value'] + owner_extent > owner.sections[own['section']]['size']:
        raise ValueError('Compiler owner extent exceeds its data section')
    if native_owner + offset != native_field:
        raise ValueError('Compiler field offset disagrees with native binding')
    result = append_label(owner, name, own['section'], own['value'] + offset)
    return result, {'name': name, 'owner': spec['owner'], 'offset': offset,
                    'size': extent, 'ownerSize': owner_extent}
