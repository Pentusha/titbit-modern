// Compiler-generated template-static guards (locale::id of ctype<unsigned short>, and codecvt<char,char,int> in <fstream> units) of the 0x43xxxx CenterPoint units; stand-in for the per-unit $E code (the LINK.EXE build gets these from the real units and should omit this file).
extern "C" {
extern unsigned char g_005f4500;
extern unsigned char g_005f44f4;
int __cdecl FUN_004a8887(void(__cdecl *)(void));   // atexit
void __cdecl FUN_00404d10(void);
}
#define GUARD_INIT(head, tail) \
 extern "C" void __cdecl tail(void); \
 extern "C" void __cdecl head(void) { if (!(g_005f4500 & 1)) g_005f4500 |= 1; tail(); } \
 extern "C" void __cdecl tail(void) { FUN_004a8887(FUN_00404d10); }
#define GUARD_INIT2(head, tail1, tail2) \
 extern "C" void __cdecl tail1(void); \
 extern "C" void __cdecl tail2(void); \
 extern "C" void __cdecl head(void) { \
  if (!(g_005f4500 & 1)) g_005f4500 |= 1; tail1(); \
  if (!(g_005f44f4 & 1)) g_005f44f4 |= 1; tail2(); } \
 extern "C" void __cdecl tail1(void) { FUN_004a8887(FUN_00404d10); } \
 extern "C" void __cdecl tail2(void) { FUN_004a8887(FUN_00404d10); }
GUARD_INIT2(LocaleIdGuard_0043b6f0, LocaleIdAtexit_0043b720, LocaleIdAtexit_0043b730)
GUARD_INIT2(LocaleIdGuard_0043bc50, LocaleIdAtexit_0043bc80, LocaleIdAtexit_0043bc90)
GUARD_INIT(LocaleIdGuard_0043bf40, LocaleIdAtexit_0043bf60)
GUARD_INIT(LocaleIdGuard_0043bfa0, LocaleIdAtexit_0043bfc0)
GUARD_INIT(LocaleIdGuard_0043bfd0, LocaleIdAtexit_0043bff0)
GUARD_INIT(LocaleIdGuard_0043c700, LocaleIdAtexit_0043c720)
GUARD_INIT(LocaleIdGuard_0043c730, LocaleIdAtexit_0043c750)
GUARD_INIT(LocaleIdGuard_0043d4a0, LocaleIdAtexit_0043d4c0)
GUARD_INIT(LocaleIdGuard_0043f440, LocaleIdAtexit_0043f460)
GUARD_INIT(LocaleIdGuard_0043f470, LocaleIdAtexit_0043f490)
