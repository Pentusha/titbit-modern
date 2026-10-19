// Static-object initializers and atexit destructors of the 0x41xxxx units.
extern "C" {
extern unsigned char g_005f4500;
int __cdecl FUN_004a8887(void(__cdecl *)(void));   // atexit
void __cdecl FUN_00404d10(void);
}
#define GUARD_INIT(head, tail) \
 extern "C" void __cdecl tail(void); \
 extern "C" void __cdecl head(void) { if (!(g_005f4500 & 1)) g_005f4500 |= 1; tail(); } \
 extern "C" void __cdecl tail(void) { FUN_004a8887(FUN_00404d10); }
GUARD_INIT(Script_CXX_Initializer_00411d40, FUN_00411d60)
GUARD_INIT(Script_CXX_Initializer_004120f0, FUN_00412110)
GUARD_INIT(Script_CXX_Initializer_00413780, FUN_004137a0)
GUARD_INIT(Script_CXX_Initializer_00413f60, FUN_00413f80)
GUARD_INIT(Script_CXX_Initializer_00415d50, FUN_00415d70)
GUARD_INIT(Script_CXX_Initializer_00416870, FUN_00416890)
GUARD_INIT(Script_CXX_Initializer_0041a700, FUN_0041a720)
GUARD_INIT(Script_CXX_Initializer_0041b330, FUN_0041b350)
GUARD_INIT(Script_CXX_Initializer_0041d9d0, FUN_0041d9f0)
GUARD_INIT(Script_CXX_Initializer_0041e550, FUN_0041e570)

// Global array of 0x78-byte records {count 0x5024a4, items 0x5024a8}.
struct Record78_00417990 { char body[0x78]; ~Record78_00417990(); };
extern "C" {
extern unsigned char g_0050249c;
extern int g_005024a4;
extern Record78_00417990 *g_005024a8;
void __cdecl Script_CXX_Atexit_00413fd0(void) {
 if (!(g_0050249c & 1)) {
  g_0050249c |= 1;
  if (g_005024a8) {
   Record78_00417990 *p = g_005024a8;
   for (int n = g_005024a4; n; n--, p++) p->~Record78_00417990();
   operator delete(g_005024a8);
   g_005024a8 = 0;
  }
  g_005024a4 = 0;
 }
}
void __cdecl FUN_00413fc0(void) { FUN_004a8887(Script_CXX_Atexit_00413fd0); }
}

