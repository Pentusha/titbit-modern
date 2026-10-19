// Guard initializers of the 0x40xxxx units: every unit that includes the engine
// header with the guarded empty static emits one (atexit of the empty dtor 0x404d10).
extern "C" {
extern unsigned char g_005f4500;
int __cdecl FUN_004a8887(void(__cdecl *)(void));   // atexit
void __cdecl FUN_00404d10(void);
}
#define GUARD_INIT(head, tail) \
 extern "C" void __cdecl tail(void); \
 extern "C" void __cdecl head(void) { if (!(g_005f4500 & 1)) g_005f4500 |= 1; tail(); } \
 extern "C" void __cdecl tail(void) { FUN_004a8887(FUN_00404d10); }
GUARD_INIT(Script_CXX_Initializer_00404ce0, FUN_00404d00)
GUARD_INIT(Script_CXX_Initializer_004051f0, FUN_00405210)
GUARD_INIT(Script_CXX_Initializer_00405de0, FUN_00405e00)
GUARD_INIT(Script_CXX_Initializer_00405e10, FUN_00405e30)
GUARD_INIT(Script_CXX_Initializer_00405e40, FUN_00405e60)
GUARD_INIT(Script_CXX_Initializer_00406730, FUN_00406750)
GUARD_INIT(Script_CXX_Initializer_00407f30, FUN_00407f50)
GUARD_INIT(Script_CXX_Initializer_0040a830, FUN_0040a850)
GUARD_INIT(Script_CXX_Initializer_0040b2f0, FUN_0040b310)
GUARD_INIT(Script_CXX_Initializer_0040c370, FUN_0040c390)
GUARD_INIT(Script_CXX_Initializer_0040c6a0, FUN_0040c6c0)
GUARD_INIT(Script_CXX_Initializer_0040e2c0, FUN_0040e2e0)
GUARD_INIT(Script_CXX_Initializer_0040f360, FUN_0040f380)
GUARD_INIT(Script_CXX_Initializer_0040fe00, FUN_0040fe20)

// Initializer-table entry that only forwards to an empty function (0x402db0 -> 0x402dc0).
extern "C" void __cdecl FUN_00402dc0(void);
extern "C" void __cdecl FUN_00402db0(void) { FUN_00402dc0(); }
