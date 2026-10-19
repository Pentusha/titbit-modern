// Global cache of 0x434-byte skin/sound records at 0x502490 {step, count, data}
// (static initializer/atexit pair 0x40e2f0..0x40e330).
void __cdecl operator delete(void *);

class SoundHolder_0040e380 {          // vtable 0x4cd56c (see lanes/claude/engine_40c.cpp)
public:
 char body[0x430];
 virtual ~SoundHolder_0040e380();
};

inline void DestructElements(SoundHolder_0040e380 *p, int n) {
 for (; n--; p++) p->~SoundHolder_0040e380();
}

class SkinCache_00502490 {
public:
 int step;                            // +0x00
 int count;                           // +0x04
 SoundHolder_0040e380 *data;          // +0x08
 SkinCache_00502490() { data = 0; step = 5; }
 ~SkinCache_00502490() {
  if (data) {
   DestructElements(data, count);
   operator delete(data);
   data = 0;
  }
  count = 0;
 }
};

SkinCache_00502490 g_00502490;
