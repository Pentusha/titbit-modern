// Record caches {step, count, data} of the 0x40xxxx units: walk areas
// (0x138b8 bytes, cache at 0x502480) and skin/sound records (0x434 bytes,
// cache at 0x502490). Lookup/At: lanes/claude/string_ops.cpp, engine_408.cpp.
extern "C" {
void *__cdecl memset(void *, int, unsigned);
void *__cdecl memcpy(void *, const void *, unsigned);
}
#pragma intrinsic(memset, memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
inline void *__cdecl operator new(unsigned, void *place) { return place; }
inline void __cdecl operator delete(void *, void *) {}

struct WalkArea_00407ff0 {
 char name[0x20];
 int used, refs, age;                 // +0x20 (cache bookkeeping)
 char body[0x138b8 - 0x2c];
 WalkArea_00407ff0();
 ~WalkArea_00407ff0();
};

void __stdcall ConstructWalkAreas_00407f60(WalkArea_00407ff0 *p, int n) {
 if (p && n) {
  memset(p, 0, n * sizeof(WalkArea_00407ff0));
  for (; n--; p++) ::new((void *)p) WalkArea_00407ff0;
 }
}

class WalkAreaCache_00502480 {
public:
 int step;                            // +0x00
 int count;                           // +0x04
 WalkArea_00407ff0 *data;             // +0x08
 int FindFree_00407e50();
 bool Reserve_00407eb0(int size);
};

// Reuse the last free slot, or the first idle area younger than 1000.
int WalkAreaCache_00502480::FindFree_00407e50() {
 int found = -1;
 for (int i = 1; i < count - 1; i++) {
  if (data[i].used == 0) { found = i; continue; }
  if (data[i].age < 1000 && data[i].refs == 0) { found = i; break; }
 }
 if (found != -1) data[found].~WalkArea_00407ff0();
 return found;
}

bool WalkAreaCache_00502480::Reserve_00407eb0(int size) {
 if (count >= size) return false;
 WalkArea_00407ff0 *next = (WalkArea_00407ff0 *)operator new(size * sizeof(WalkArea_00407ff0));
 memcpy(next, data, count * sizeof(WalkArea_00407ff0));
 if (data) operator delete(data);
 data = next;
 ConstructWalkAreas_00407f60(&next[count], size - count);
 count = size;
 return true;
}

class SoundHolder_0040e380 {          // vtable 0x4cd56c
public:
 char body[0x430];
 SoundHolder_0040e380();
 virtual ~SoundHolder_0040e380();
};

void __stdcall ConstructSkins_0040a860(SoundHolder_0040e380 *p, int n) {
 if (p && n) {
  memset(p, 0, n * sizeof(SoundHolder_0040e380));
  for (; n--; p++) ::new((void *)p) SoundHolder_0040e380;
 }
}

class SkinCache_00502490 {
public:
 int step;                            // +0x00
 int count;                           // +0x04
 SoundHolder_0040e380 *data;          // +0x08
 bool Reserve_0040a790(int size);
};

bool SkinCache_00502490::Reserve_0040a790(int size) {
 if (count >= size) return false;
 SoundHolder_0040e380 *next = (SoundHolder_0040e380 *)operator new(size * sizeof(SoundHolder_0040e380));
 memcpy(next, data, count * sizeof(SoundHolder_0040e380));
 if (data) operator delete(data);
 data = next;
 ConstructSkins_0040a860(&next[count], size - count);
 count = size;
 return true;
}
