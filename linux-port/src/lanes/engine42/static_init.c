/* Static-object initializers and atexit wrappers in 0x420000-0x42ffff.
 * The guarded (byte|=bit) forms are the per-TU copies VC6 emits for objects
 * defined in shared headers; the shared one at 0x5f4500 has an empty dtor. */
extern int __cdecl FUN_004a8887(void (__cdecl *)(void));   /* atexit */
extern void __cdecl FUN_004a7e2a(void *);                   /* operator delete */
extern void __cdecl FUN_00404d10(void);                     /* empty dtor */
extern unsigned char g_005f4500;

#define SHARED_INIT(init, reg) \
void __cdecl reg(void) { FUN_004a8887(FUN_00404d10); } \
void __cdecl init(void) { if (!(g_005f4500 & 1)) g_005f4500 |= 1; reg(); }
SHARED_INIT(FUN_00420310, FUN_00420330)
SHARED_INIT(FUN_004266e0, FUN_00426700)
SHARED_INIT(FUN_004268f0, FUN_00426910)
SHARED_INIT(FUN_004277c0, FUN_004277e0)
SHARED_INIT(FUN_004282e0, FUN_00428300)
SHARED_INIT(FUN_00428a80, FUN_00428aa0)
SHARED_INIT(FUN_0042a230, FUN_0042a250)
SHARED_INIT(FUN_0042a3f0, FUN_0042a410)
SHARED_INIT(FUN_0042cb50, FUN_0042cb70)
SHARED_INIT(FUN_0042cb80, FUN_0042cba0)
SHARED_INIT(FUN_0042cd00, FUN_0042cd20)
SHARED_INIT(FUN_0042e9d0, FUN_0042e9f0)
SHARED_INIT(FUN_0042ea00, FUN_0042ea20)

/* scene list object 0x5026d0 (ctor 0x41e8f0, empty dtor 0x4051b0) */
extern unsigned char g_005026cc;
extern char g_005026d0[];
extern void __fastcall FUN_0041e8f0(void *);
extern void __fastcall FUN_004051b0(void *);
void __cdecl FUN_00421590(void) { FUN_0041e8f0(g_005026d0); }
void __cdecl FUN_004215b0(void) { if (!(g_005026cc & 1)) { g_005026cc |= 1; FUN_004051b0(g_005026d0); } }
void __cdecl FUN_004215a0(void) { FUN_004a8887(FUN_004215b0); }
void __cdecl FUN_00421580(void) { FUN_00421590(); FUN_004215a0(); }

/* array object 0x5ee704 (ctor 0x427870, dtor 0x4278b0) */
extern unsigned char g_005ee700;
extern char g_005ee704[];
extern void __fastcall FUN_00427870(void *);
extern void __fastcall FUN_004278b0(void *);
void __cdecl FUN_00427d80(void) { FUN_00427870(g_005ee704); }
void __cdecl FUN_00427da0(void) { if (!(g_005ee700 & 1)) { g_005ee700 |= 1; FUN_004278b0(g_005ee704); } }
void __cdecl FUN_00427d90(void) { FUN_004a8887(FUN_00427da0); }
void __cdecl FUN_00427d70(void) { FUN_00427d80(); FUN_00427d90(); }

/* buffer 0x5ee71c..0x5ee728 */
extern void *g_005ee71c;
extern int g_005ee720, g_005ee724, g_005ee728;
void __cdecl FUN_004298e0(void) { g_005ee71c = 0; g_005ee728 = 0; g_005ee724 = 0; g_005ee720 = 0; }
void __cdecl FUN_00429910(void) { if (g_005ee71c) FUN_004a7e2a(g_005ee71c); }
void __cdecl FUN_00429900(void) { FUN_004a8887(FUN_00429910); }
void __cdecl FUN_004298d0(void) { FUN_004298e0(); FUN_00429900(); }

/* identity matrix global 0x502678 (inline ObservedMatrix4::Identity) */
extern float g_00502678[16];
void __cdecl FUN_004215e0(void) {
 float *v = g_00502678;
 v[0] = v[5] = v[10] = v[15] = 1;
 v[1] = v[2] = v[3] = v[4] = v[6] = v[7] = v[8] = v[9] = v[11] = v[12] = v[13] = v[14] = 0;
}
void __cdecl FUN_004215d0(void) { FUN_004215e0(); }
