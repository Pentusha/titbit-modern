// Scripted game object (vtable 0x4cd570, 0x8c bytes): a skeleton with a SeeR
// script holder as second base. Script functions that create game objects and
// attach animations/timers to them.
#define E40_LOADSTRING_CALL   // CString::LoadString 0x406520 out of line (0x40ecd0)
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
#ifdef TITBIT_SOURCE_LINK
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int);
 virtual void Render(int);
#endif
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

GameObject_0040e630::GameObject_0040e630() { selected = false; }
GameObject_0040e630::~GameObject_0040e630() {}

// Scene loader entry for a game object's bones and animations.
void GameObject_0040e630::Load_0040e710(XmlNode_v *node) { Load_0041db90(node); }

struct ExtraChain : ObjectExtra { int owner; ExtraChain *next; void Append_0040b270(ExtraChain *item); };

struct SkeletonAnimation : ExtraChain {  // 0x4269f0 (was Animation_004269f0)
 char body[0x2c - 0x0c];
 int loops;                           // +0x2c
 ObservedString file;                 // +0x30
 SkeletonAnimation(EngineObject *owner);
 bool LoadFile_00427300(ObservedString path);
};


struct StringReplace { char *data; int Replace_0040eff0(const char *old_text, const char *new_text); };

struct SceneMessage { char head[0x20]; void *object; int code; char tail[0x48 - 0x28]; };
struct Scene_005011c0 { bool Send_00424780(SceneMessage *message); };
extern Scene_005011c0 g_005011c0;

// GAMEOBJECT.Create script function: bone 0 at (x, y, z) with half extents
// (sx, sy, sz) / 2; the object is named after its script file without ".scv".
GameObject_0040e630 *__cdecl CreateGameObject_0040ecd0(int bones, const char *script, float x, float y, float z,
                                                         float sx, float sy, float sz) {
 GameObject_0040e630 *object = new GameObject_0040e630;
 if (bones > 0) {
  object->bones = new Bone_0040c6d0[bones];
  float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
  object->bones[0].position.x = x;
  object->bones[0].position.y = y;
  object->bones[0].position.z = z;
  object->bone_count = bones;
  object->bones[0].v38.x = hx;
  object->bones[0].v38.y = hy;
  object->bones[0].v38.z = hz;
  object->bones[0].center.z = hz;
  object->bones[0].center.x = hx;
  object->bones[0].center.y = hy;
  for (int i = 1; i < bones; i++) object->bones[i].parent = 0;
 }
 object->BindScript_0040e720(script);
 ObservedString name;
 name = script;
 ((StringReplace *)&name)->Replace_0040eff0(".scv", g_00500ef0);   // out-of-line CString::Replace, as in the original
 strcpy(object->name, name);
 SceneMessage message;
 memset(&message, 0, sizeof(message));
 message.code = 3;
 message.object = object;
 g_005011c0.Send_00424780(&message);
 object->visible = 1;
 object->selected = true;
 return object;
}

struct Timer : ObjectExtra {  // 0x42ca70 (was Timer_0042ca70)
 int owner, field8, fieldc, field10, field14;
 int id;                              // +0x18
 Timer(int id, int now, int interval, int count);
};

Timer *GameObject_0040e630::AddTimer_0040ef70(int interval, int count) {
 Timer *timer = new Timer(1, g_005026bc, interval, count);
 AttachExtra_0041e530(timer);
 timer->id = (int)timer;
 return timer;
}
