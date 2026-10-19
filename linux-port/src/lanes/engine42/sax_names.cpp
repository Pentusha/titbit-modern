#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Arabica SAX name strings kept in three global std::string objects with a shared
// destruction guard byte (0x42ef60-0x42f260).
#pragma warning(disable: 4786)
#include <string>
typedef std::string string42;
extern "C" int __cdecl atexit(void (__cdecl *)(void));

extern std::string g_005ee740;      // empty
extern std::string g_005ee750;      // "http://xml.org/sax/handlers/LexicalHandler"
extern std::string g_005ee72c;      // "namespaces"
extern unsigned char g_005ee73c;    // destruction guard bits

void __cdecl FUN_0042ef70() { new ((void *)&g_005ee740) std::string; }
void __cdecl FUN_0042efa0() { if (!(g_005ee73c & 1)) { g_005ee73c |= 1; g_005ee740.~string42(); } }
void __cdecl FUN_0042ef90() { atexit(FUN_0042efa0); }
void __cdecl FUN_0042ef60() { FUN_0042ef70(); FUN_0042ef90(); }

std::string __cdecl MakeString_0045adc0(const std::string &text);
void __cdecl FUN_0042f010() { new ((void *)&g_005ee750) std::string(MakeString_0045adc0(std::string("http://xml.org/sax/handlers/LexicalHandler"))); }
void __cdecl FUN_0042f100() { if (!(g_005ee73c & 2)) { g_005ee73c |= 2; g_005ee750.~string42(); } }
void __cdecl FUN_0042f0f0() { atexit(FUN_0042f100); }
void __cdecl FUN_0042f000() { FUN_0042f010(); FUN_0042f0f0(); }

void __cdecl FUN_0042f170() { new ((void *)&g_005ee72c) std::string(MakeString_0045adc0(std::string("namespaces"))); }
void __cdecl FUN_0042f260() { if (!(g_005ee73c & 4)) { g_005ee73c |= 4; g_005ee72c.~string42(); } }
void __cdecl FUN_0042f250() { atexit(FUN_0042f260); }
void __cdecl FUN_0042f160() { FUN_0042f170(); FUN_0042f250(); }
