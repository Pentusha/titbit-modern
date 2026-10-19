// GAME_OBJECT.CreateSkeletonAnimation (0x40eb10), in its own unit: here VC6
// expanded CString::LoadString inline, while the rest of game_object.cpp
// calls it out of line (E40_LOADSTRING_CALL there).
#include "e40_engine.h"
extern "C" {
char *__cdecl strcpy(char *, const char *);
void *__cdecl memset(void *, int, unsigned);
extern char g_00500ef0[];           // ""
extern int g_005026bc;              // scene time
}
#pragma intrinsic(strcpy, memset)

struct Quat4Z { float x, y, z, w; Quat4Z() { x = y = z = w = 0; } };
struct Mat4I {
 float m[16];
 Mat4I() {
  m[0] = m[5] = m[10] = m[15] = 1.0f;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
 }
};
struct Bone_0040c6d0 {                // 0xd8 bytes
 int flags;                         // +0
 Vec3Z position;                    // +4
 Quat4Z rotation;                   // +0x10
 Vec3Z scale;                       // +0x20
 Vec3Z center;                      // +0x2c
 Vec3Z v38, v44;                    // +0x38, +0x44
 int link;                          // +0x50
 Mat4I local, world;                // +0x54, +0x94
 int parent;                        // +0xd4
 Bone_0040c6d0();
 ~Bone_0040c6d0() {}                 // declared: new[] stores the element count (0x40ecd0 allocates n*0xd8+4)
};

class Skeleton : public EngineObject { // original 0x41da00
public:
 char name[0x20];                     // +0x14
 int bone_count;                      // +0x34
 Bone_0040c6d0 *bones;                // +0x38
 float depth;                         // +0x3c
 int visible;                         // +0x40
 Skeleton();
 virtual ~Skeleton();
 void Load_0041db90(struct XmlNode_v *node);

};

class ScriptHolder { // original 0x41a770
public:
 void *program;                       // +0x04
 void *instance;                      // +0x08
 int handlers[13];                    // +0x0c
 ScriptHolder();
 virtual ~ScriptHolder();
};

class GameObject_0040e630 : public Skeleton, public ScriptHolder {
public:
 int field84;
 bool selected;                       // +0x88
 GameObject_0040e630();
 virtual ~GameObject_0040e630();
 bool BindScript_0040e720(ObservedString source);
 void Load_0040e710(struct XmlNode_v *node);
 struct SkeletonAnimation *AddAnimation_0040eb10(const char *file, int loops);  // 0x4269f0 (was Animation_004269f0)
 struct Timer *AddTimer_0040ef70(int interval, int count);  // 0x42ca70 (was Timer_0042ca70)
};

struct ExtraChain : ObjectExtra { int owner; ExtraChain *next; void Append_0040b270(ExtraChain *item); };

struct SkeletonAnimation : ExtraChain {  // 0x4269f0 (was Animation_004269f0)
 char body[0x2c - 0x0c];
 int loops;                           // +0x2c
 ObservedString file;                 // +0x30
 SkeletonAnimation(EngineObject *owner);
 bool LoadFile_00427300(ObservedString path);
};

SkeletonAnimation *GameObject_0040e630::AddAnimation_0040eb10(const char *file, int loops) {
 SkeletonAnimation *animation = new SkeletonAnimation(this);
 if (!animation->LoadFile_00427300(file)) {
  if (animation) animation->Release(1);
  animation = 0;
 } else {
  if (extra == 0) extra = animation;
  else ((ExtraChain *)extra)->Append_0040b270(animation);
  animation->loops = loops;
 }
 return animation;
}
