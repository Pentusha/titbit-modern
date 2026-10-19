// Global record caches: animation/mesh records (0x78 bytes, cache at 0x5024a0)
// and resource records (0x74 bytes, cache at 0x5ee6dc).
#include "e41_string.h"
void __cdecl PickingTrace(const char *, ...);               // engine log, 0x403650
extern "C" {
__declspec(dllimport) char *__stdcall CharLowerA(char *);
__declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
void *__cdecl memset(void *, int, unsigned);
void *__cdecl memcpy(void *, const void *, unsigned);
}
#pragma intrinsic(memset, memcpy)
inline void *__cdecl operator new(unsigned, void *place) { return place; }
inline void __cdecl operator delete(void *, void *) {}

struct NamedHeader_00402ce0 { char body[0x2c]; NamedHeader_00402ce0(); };

struct AnimRecord { // original 0x417940
 NamedHeader_00402ce0 header;         // +0x00
 int state2c;                         // +0x2c
 void *buffer30;                      // +0x30
 int unknown34;
 void *buffer38;                      // +0x38
 int state3c;                         // +0x3c
 void *buffer40, *buffer44;           // +0x40
 int state48;                         // +0x48
 void *buffer4c, *buffer50, *buffer54;// +0x4c
 int reserved58[4];
 void *buffer68, *buffer6c;           // +0x68
 int *resources;                      // +0x70
 int resource_count;                  // +0x74
 AnimRecord();
 ~AnimRecord();               // 0x417980: jumps to Clear
 void Clear_00417990();
};

AnimRecord::AnimRecord() {
 buffer38 = 0;
 buffer50 = 0;
 buffer40 = 0;
 buffer54 = 0;
 buffer6c = 0;
 buffer44 = 0;
 buffer4c = 0;
 buffer30 = 0;
 buffer68 = 0;
 state48 = 0;
 state3c = 0;
 state2c = 0;
}

class VfsFile {  // 0x4203d0 (was VfsFile_004203d0)
public:
 ObservedString name;
 unsigned position;
 void *handle;
 unsigned size;
 int source, kind;
 VfsFile();
 ~VfsFile();
 int Locate_00420420(const char *path);
 void Open_004206b0(const char *path);
 unsigned Read_004209b0(void *buffer, unsigned count);
 void Close_00420a60();
};
struct TextureCache { int Load_0042c2b0(ObservedString name, int flags); }; // original global 0x5ee6f0  // 0x42c080 (was TextureCache_0042c080)
extern TextureCache g_005ee6f0;
extern "C" int __cdecl strcmp(const char *, const char *);
extern "C" char *__cdecl strcpy(char *, const char *);
#pragma intrinsic(strcmp, strcpy)

struct AnimSlot { char name[0x20]; int used, refs, age; };   // view of the cache-relevant fields

void __stdcall ConstructAnimRecords_00415d80(AnimRecord *p, int n) {
 if (p && n) {
  memset(p, 0, n * sizeof(AnimRecord));
  for (; n--; p++) ::new((void *)p) AnimRecord;
 }
}

class AnimationCache { // original global 0x5024a0
public:
 int step;                            // +0x00 (5)
 int count;                           // +0x04
 AnimRecord *data;           // +0x08
 int FindFree_00415c60();
 bool Reserve_00415cc0(int size);
 int Find_00415b40(ObservedString name);
 int Load_004158c0(ObservedString path, int flags);
};

// Reuse the last free slot, or the first idle record younger than 1000.
int AnimationCache::FindFree_00415c60() {
 int found = -1;
 for (int i = 1; i < count - 1; i++) {
  AnimSlot *r = (AnimSlot *)&data[i];
  if (r->used == 0) { found = i; continue; }
  if (r->age < 1000 && r->refs == 0) { found = i; break; }
 }
 if (found != -1) data[found].~AnimRecord();
 return found;
}

bool AnimationCache::Reserve_00415cc0(int size) {
 if (count >= size) return false;
 AnimRecord *next = (AnimRecord *)operator new(size * sizeof(AnimRecord));
 memcpy(next, data, count * sizeof(AnimRecord));
 if (data) operator delete(data);
 data = next;
 ConstructAnimRecords_00415d80(&next[count], size - count);
 count = size;
 return true;
}

struct ResourceSlot {
 char name[0x20];
 int used, refs, age;                 // +0x20
 float ambient[3], diffuse[3], specular[3];   // +0x2c
 float shininess, value54, value58;   // +0x50
 int blend;                           // +0x5c
 unsigned texture, texture2, texture3;// +0x60
 char two_sided; char pad6d[3];       // +0x6c
 unsigned env_texture;                // +0x70
 void Clear_004168a0(); void Reset_004178f0(); bool Load_004168f0(ObservedString &file, int flags, int options); };

class ResourceCache { // original global 0x5ee6dc
public:
 ResourceSlot *data;                  // +0x04
 int count;                           // +0x08
 int used;                            // +0x0c
 int step;                            // +0x10
 virtual ~ResourceCache();
 ResourceSlot *Get_00417270(int index);
 ResourceSlot *Find_004172a0(ObservedString &name);
 int FindFree_00417740();
 bool Resize_004177b0(int size);
 int Load_00417390(ObservedString &file, int flags, int options);
 int Count_00417920();
};

ResourceSlot *ResourceCache::Get_00417270(int index) {
 if (index < count) return &data[index];
 return 0;
}

int ResourceCache::FindFree_00417740() {
 int best = -1, oldest = 0xffff;
 for (int i = 1; i < count; i++) {
  if (data[i].used == 0) return i;
  if (data[i].age < oldest && data[i].refs == 0) { oldest = data[i].age; best = i; }
 }
 if (best != -1) data[best].Clear_004168a0();
 return best;
}

ResourceSlot *ResourceCache::Find_004172a0(ObservedString &name) {
 name.MakeLower();
 for (int i = 0; i < count; i++)
  if (lstrcmpA(name, (const char *)&data[i]) == 0) return &data[i];
 return 0;
}

extern "C" void __cdecl FUN_00413fa0(void);
extern "C" void __cdecl FUN_00413fc0(void);
extern "C" void __cdecl Script_CXX_Initializer_00413f90(void) { FUN_00413fa0(); FUN_00413fc0(); }

int AnimationCache::Find_00415b40(ObservedString name) {
 name.MakeLower();
 for (int i = 0; i < count; i++)
  if (lstrcmpA(name, ((AnimSlot *)&data[i])->name) == 0) return i;
 return -1;
}

// Register a mesh by file name (directory and extension stripped); returns
// the slot of an already loaded one with its reference count raised.
int AnimationCache::Load_004158c0(ObservedString path, int flags) {
 path.MakeLower();
 ObservedString name;
 name = path.Mid(path.ReverseFind('\\') + 1);
 name = name.Mid(0, name.ReverseFind('.'));
 int index = Find_00415b40(name);
 if (index == -1) {
  index = FindFree_00415c60();
  if (index == -1) {
   index = count;
   Reserve_00415cc0(step + index);
  }
  ((AnimSlot *)&data[index])->used = index;
  ObservedString file(path);
  return -1;
 }
 ((AnimSlot *)&data[index])->refs++;
 return index;
}

bool ResourceCache::Resize_004177b0(int size) {
 if (size <= count) return false;
 ResourceSlot *next = (ResourceSlot *)operator new(size * sizeof(ResourceSlot));
 memcpy(next, data, count * sizeof(ResourceSlot));
 if (data) operator delete(data);
 data = next;
 for (int i = count; i < size; i++) data[i].Reset_004178f0();
 count = size;
 ObservedString message;
 message.Format("Materials hostsize is %d", size);
 PickingTrace(message);
 return true;
}

// Material by file name (directory and extension stripped); new ones are
// loaded into a free or newly grown slot.
int ResourceCache::Load_00417390(ObservedString &file, int flags, int options) {
 ObservedString name, message;
 name = file.Mid(file.ReverseFind('\\') + 1);
 name = name.Mid(0, name.ReverseFind('.'));
 ResourceSlot *slot = Find_004172a0(name);
 if (slot == 0) {
  int index = FindFree_00417740();
  if (index == -1) {
   index = count;
   if (count == 0) index = 1;
   Resize_004177b0(step + count);
  }
  PickingTrace(message);
  data[index].used = index;
  data[index].refs = 1;
  if (!data[index].Load_004168f0(file, flags, options)) return 0;
  return index;
 }
 data[slot->used].refs++;
 return slot->used;
}

static inline void UnpackColor(float *out, unsigned c) {
 out[0] = (float)(c & 0xff) * (1.0f / 255.0f);
 out[1] = (float)(c >> 8 & 0xff) * (1.0f / 255.0f);
 out[2] = (float)(c >> 16 & 0xff) * (1.0f / 255.0f);
}

// Material file: "MAT" magic, version, three packed RGB colours, three floats,
// map flags (1 diffuse, 2 second layer, 4 third layer, 8 environment).
bool ResourceSlot::Load_004168f0(ObservedString &file, int flags, int options) {
 ObservedString texture, dir;
 char buffer[32];
 memset(buffer, 0, sizeof(buffer));
 PickingTrace("Start reading material");
 VfsFile in;
 if (!in.Locate_00420420(file)) return false;
 in.Open_004206b0(file);
 in.Read_004209b0(buffer, 3);
 buffer[3] = 0;
 if (strcmp(buffer, "MAT")) { in.Close_00420a60(); return false; }
 file.MakeLower();
 file.Replace(".mat", "");
 int slash = file.ReverseFind('\\');
 dir = file.Mid(0, slash + 1);
 file = file.Mid(slash + 1, file.GetLength());
 strcpy(name, file);
 int version;
 in.Read_004209b0(&version, 4);
 if (version > 0) {
  unsigned c;
  in.Read_004209b0(&c, 4); UnpackColor(ambient, c);
  in.Read_004209b0(&c, 4); UnpackColor(diffuse, c);
  in.Read_004209b0(&c, 4); UnpackColor(specular, c);
  in.Read_004209b0(&shininess, 4);
  in.Read_004209b0(&value54, 4);
  in.Read_004209b0(&value58, 4);
  unsigned char maps;
  in.Read_004209b0(&maps, 1);
  if (version > 1) {
   char b;
   in.Read_004209b0(&b, 1);
   blend = b == 1 ? 1 : 0;
   in.Read_004209b0(&b, 1);
   two_sided = b == 1 ? 1 : 0;
  }
  if (maps & 1) { in.Read_004209b0(buffer, 32); texture.Format("%s", buffer); this->texture = g_005ee6f0.Load_0042c2b0(dir + texture, 3); }
  if (maps & 2) { in.Read_004209b0(buffer, 32); texture.Format("%s", buffer); texture2 = g_005ee6f0.Load_0042c2b0(dir + texture, 3); }
  if (maps & 4) { in.Read_004209b0(buffer, 32); texture.Format("%s", buffer); texture3 = g_005ee6f0.Load_0042c2b0(dir + texture, 3); }
  if (maps & 8) { in.Read_004209b0(buffer, 32); texture.Format("%s", buffer); env_texture = g_005ee6f0.Load_0042c2b0(dir + texture, 3); }
  in.Close_00420a60();
  PickingTrace("Read complete");
  return true;
 }
 in.Close_00420a60();
 return false;
}
