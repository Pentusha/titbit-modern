"""Make the existing authored leaf C sources relocatable for a real PE link.

Preserve the original sources. Image DATA address constants become symbolic
addresses with undefined extern declarations, never copied payload or storage.
Declarations are deliberately byte-typed: the original expression retains its
actual access width/volatile qualification. A source provider must satisfy LINK.
"""
import re


TOKENS = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\b0[xX][0-9a-fA-F]+[uUlL]*\b', re.S)


def relocate(text, data_ranges):
    references = set()

    def replace(match):
        token = match.group()
        if not token.lower().startswith('0x'):
            return token
        address = int(token.rstrip('uUlL'), 16)
        if not 0x400000 <= address < 0x600000:
            return token
        if not any(start <= address < end for start, end in data_ranges):
            raise ValueError('Leaf image constant is not independently mapped DATA: ' + token)
        references.add(address)
        return '((unsigned int)&g_%08x)' % address

    body = TOKENS.sub(replace, text)
    declarations = ''.join('extern unsigned char g_%08x;\n' % address for address in sorted(references))
    return '/* Relocatable build adaptation; see the original src/leaf source. */\n' + declarations + body, sorted(references)
