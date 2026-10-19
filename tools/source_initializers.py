"""Source-owned callback sequences for the standalone executable.

Per-TU automatic XI/XC/XP/XT tables are excluded from LINK after their compiler-
emitted callback code is preserved. Native callback identities/order are
independently recorded metadata; all pointers refer to source definitions.
"""
import struct

from coff_link import CoffObject


def bind_static_callbacks(coff, fields):
    """Identify source XCU roots by the real static member and its literal.

    Compiler _$E ordinals change when the correct game header adds a field.
    Follow only local initializer helpers, never an assumed native operand.
    """
    roots = [r['symbol'] for s in coff.sections.values() if s['name'] == '.CRT$XCU'
             for r in s['relocations']]

    def references(symbol, seen):
        if symbol['index'] in seen or symbol['section'] <= 0:
            return set(), set()
        seen.add(symbol['index'])
        section = coff.sections[symbol['section']]
        end = min([s['value'] for s in coff.symbols.values() if s['section'] == symbol['section']
                   and s['type'] & 0x20 and s['value'] > symbol['value']] + [section['size']])
        found, literals = set(), set()
        for relocation in section['relocations']:
            if not symbol['value'] <= relocation['offset'] < end:
                continue
            target = relocation['symbol']
            if target['name'] in fields:
                found.add(target['name'])
            if target['name'].startswith('??_C@') and target['section'] > 0:
                data = coff.sections[target['section']]['bytes'][target['value']:]
                literals.add(data.split(b'\0', 1)[0])
            if target['name'].startswith('_$E') and target['type'] & 0x20:
                f, texts = references(target, seen)
                found.update(f)
                literals.update(texts)
        return found, literals

    hits = {name: [] for name in fields}
    for root in roots:
        found, literals = references(root, set())
        if not found:
            continue
        if len(found) != 1:
            raise ValueError('Composite static initializer needs a dedicated source mapping')
        field = next(iter(found))
        expected = fields[field].get('literal')
        if expected is not None and expected.encode('ascii') not in literals:
            raise ValueError('Static member matches but its source literal disagrees: ' + field)
        hits[field].append(root['name'])
    bindings = {}
    for field, roots in hits.items():
        if len(roots) != 1:
            raise ValueError('Missing/ambiguous source initializer for ' + field)
        bindings[roots[0]] = int(fields[field]['initializer'], 16)
        bindings[field] = int(fields[field]['address'], 16)
    return bindings


def validate_source_table(coff, addresses, symbol_name='_engine_global_initializers', section_name='.CRT$XCU'):
    symbol = coff.by_name[symbol_name]
    section = coff.sections[symbol['section']]
    if section['name'] != section_name or symbol['value'] != 0 or section['size'] != 4 * len(addresses):
        raise ValueError('Source callback array extent disagrees with native sequence')
    if any(section['bytes']):
        raise ValueError('Source table must contain relocation expressions, not native pointer payload')
    by_offset = {r['offset']: r for r in section['relocations']}
    if len(by_offset) != len(addresses) or len(section['relocations']) != len(addresses):
        raise ValueError('Source table has missing or duplicate relocations')
    for i, address in enumerate(addresses):
        relocation = by_offset.get(4 * i)
        if not relocation or relocation['kind'] != 6 or relocation['symbol']['name'] != '_source_initializer_%08x' % address:
            raise ValueError('Source callback order/identity mismatch at slot%d' % i)


def exclude_automatic_tables(coff):
    payload = bytearray(coff.data)
    optional_size = struct.unpack_from('<H', payload, 16)[0]
    section_table = 20 + optional_size
    count = 0
    for index, section in coff.sections.items():
        if not section['name'].startswith(('.CRT$XC', '.CRT$XI', '.CRT$XP', '.CRT$XT')):
            continue
        if section['name'] in tuple('.CRT$X' + kind + boundary for kind in 'CIPT' for boundary in 'AZ'):
            continue
        if section['name'] not in ('.CRT$XCU', '.CRT$XIC', '.CRT$XIY', '.CRT$XPX', '.CRT$XTB'):
            raise ValueError('Unclassified startup/termination contribution: ' + section['name'])
        if section['size'] != 4 * len(section['relocations']):
            raise ValueError('Unexpected automatic C++ startup table payload')
        count += len(section['relocations'])
        at = section_table + (index - 1) * 40 + 36
        struct.pack_into('<I', payload, at, section['flags'] | 0x800)  # IMAGE_SCN_LNK_REMOVE
    revised = CoffObject(coff.path, data=payload)
    for index, section in coff.sections.items():
        current = revised.sections[index]
        if (current['bytes'], current['size'], current['relocations']) != (section['bytes'], section['size'], section['relocations']):
            raise ValueError('Excluding automatic registration changed compiler payload')
    return revised, count
