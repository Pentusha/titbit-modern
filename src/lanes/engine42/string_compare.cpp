// Out-of-line instance of VC6 std::string::compare(const char *) (0x425f40).
// In the original it is used only by Scene::ReadCamera (0x423f80), where VC6
// kept this member out of line; elsewhere the compiler expands it inline.
#pragma warning(disable: 4786)
#include <string>

template class std::basic_string<char, std::char_traits<char>, std::allocator<char> >;   // emits compare(const char *)
