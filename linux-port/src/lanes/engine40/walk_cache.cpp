// Global cache of walk areas (0x138b8-byte records) at 0x502480 {step, count, data}:
// static initializer 0x406970, its guarded atexit destructor 0x4069b0, and the
// walk-area destructor 0x4082a0 (destroys the 1000 boundary edges).
void __cdecl operator delete(void *);
extern "C" int __cdecl FUN_004a8887(void(__cdecl *)(void));   // atexit

struct AreaEdge_00406770 { char body[0x38]; AreaEdge_00406770(); ~AreaEdge_00406770(); };

struct WalkArea_00407ff0 {
 char head[0x5df0];
 AreaEdge_00406770 edges[1000];       // 0x5df0
 int vertexCount;                     // 0x138b0
 AreaEdge_00406770 *firstEdge;        // 0x138b4
 ~WalkArea_00407ff0();
};
WalkArea_00407ff0::~WalkArea_00407ff0() {}

extern "C" {
extern unsigned char g_0050247c;
extern int g_00502480, g_00502484;
extern WalkArea_00407ff0 *g_00502488;

void __cdecl Script_CXX_Atexit_004069b0(void) {
 if (!(g_0050247c & 1)) {
  g_0050247c |= 1;
  if (g_00502488) {
   WalkArea_00407ff0 *p = g_00502488;
   for (int n = g_00502484; n; n--, p++) p->~WalkArea_00407ff0();
   operator delete(g_00502488);
   g_00502488 = 0;
  }
  g_00502484 = 0;
 }
}
void __cdecl FUN_00406980(void) { g_00502488 = 0; g_00502480 = 5; }
void __cdecl FUN_004069a0(void) { FUN_004a8887(Script_CXX_Atexit_004069b0); }
void __cdecl Script_CXX_Initializer_00406970(void) { FUN_00406980(); FUN_004069a0(); }
}
