"""Resolve an inline/out-of-line provider collision only after full equality.

Only COFF COMDAT selection metadata changes. Every compiler section byte and
relocation is preserved. Different payload or symbol operands always reject.
"""
import hashlib
import struct

from coff_link import CoffObject


def equivalent_comdat_providers(objects, name):
    providers = []
    for obj in objects:
        symbol = obj.by_name.get(name)
        if not symbol or symbol['section'] <= 0:
            continue
        if not symbol['type'] & 0x20 or symbol['value']:
            raise ValueError('COMDAT policy requires an isolated source function: ' + name)
        section = obj.sections[symbol['section']]
        if not section['flags'] & 0x1000:
            raise ValueError('Source provider is not already COMDAT: ' + name)
        definitions = [s for s in obj.symbols.values() if s['storage'] == 3 and s['aux'] == 1
                       and s['section'] == section['index'] and s['name'] == section['name']]
        if len(definitions) != 1:
            raise ValueError('Ambiguous compiler section definition: ' + name)
        table = struct.unpack_from('<I', obj.data, 8)[0]
        at = table + 18 * (definitions[0]['index'] + 1) + 14
        if obj.data[at] not in (1, 2):
            raise ValueError('Unsupported source COMDAT selection')
        identity = (section['bytes'], section['flags'], section['size'],
                    [(r['offset'], r['kind'], r['symbol']['name']) for r in section['relocations']])
        providers.append((obj, section, at, identity))
    if len(providers) < 2:
        raise ValueError('Missing alternative inline source provider: ' + name)
    reference = providers[0][3]
    if any(p[3] != reference for p in providers[1:]):
        raise ValueError('Source COMDAT providers differ in complete code or relocation identity: ' + name)
    if not any(obj.data[at] == 2 for obj, _, at, _ in providers):
        raise ValueError('No compiler-generated inline COMDAT policy to share')
    result = {}
    for obj, section, at, _ in providers:
        if obj.data[at] == 2:
            continue
        data = bytearray(obj.data)
        data[at] = 2  # Same existing compiler inline selection, verified above.
        revised = CoffObject(obj.path, data=data)
        if any((s['bytes'], s['flags'], s['relocations']) !=
               (revised.sections[i]['bytes'], revised.sections[i]['flags'], revised.sections[i]['relocations'])
               for i, s in obj.sections.items()):
            raise ValueError('COMDAT selection changed a compiler payload')
        result[str(obj.path)] = (revised, {'symbol': name, 'completeSectionBytes': section['size'],
                              'completeSectionSha256': hashlib.sha256(section['bytes']).hexdigest(),
                              'equivalentSourceProviders': len(providers)})
    return result
