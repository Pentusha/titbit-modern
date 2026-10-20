# Reconstructed C++ boundary

These independently authored headers implement the C++98 surface used by the
game. They are project code, not extracts of a compiler SDK. They are deliberately
small and are not a replacement for a general-purpose standard library.

The compiler still uses the Microsoft x86 object ABI because recovered engine
interfaces depend on it. `string` and `vector` retain their observed 16-byte
layouts. Strings allocate their own storage on copy, with a zero owner byte
preceding their character data. They do not share mutable buffers.

XML namespace contexts use an ordered linked sequence for `map`/`multimap`.
Contexts contain few bindings; stable references matter more than tree balancing.
`deque` uses a circular buffer, preserves the 48-byte owner layout, and supports
amortized constant-time insertion/removal at both ends. The recovered game does
not keep deque iterators across insertions.

Streams operate on the project's C file boundary. The supported locale is the
byte-oriented C/POSIX locale; unsupported named locales fail explicitly. No OS
locale tables or codec conversion objects are imported from VC6.

`portable/src/container_smoke.cpp` checks ownership, aliasing, nested containers,
and the layouts consumed by reconstructed XML structures. The existing native
runtime smoke test additionally exercises exceptions, streams, and file I/O.
