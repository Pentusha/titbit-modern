// Scene layout view for the engine42 lane (0x420000-0x42ffff).
// Same storage as src/native/scene_owner_view.h ObservedScene, with public
// fields so lane sources can reach them. Class name carries the ctor address.
#ifndef E42_SCENE_H
#define E42_SCENE_H
#include "../../native/scene_pair_array.h"
#include "../../native/scene_spatial.h"
#include "../../native/game_audio.h"
#include "../../native/picking_control_interfaces.h"

extern "C" {
 __declspec(dllimport) void __stdcall glEnable(unsigned);
 __declspec(dllimport) void __stdcall glDisable(unsigned);
 __declspec(dllimport) void __stdcall glPushMatrix();
 __declspec(dllimport) void __stdcall glPopMatrix();
 __declspec(dllimport) void __stdcall glLoadIdentity();
 __declspec(dllimport) void __stdcall glGetFloatv(unsigned, float *);
 __declspec(dllimport) unsigned long __stdcall timeGetTime();
}

struct Vec3_00406f00 {
 float x, y, z;
 Vec3_00406f00() {}
 Vec3_00406f00(float ax, float ay, float az) { x = ax; y = ay; z = az; }
 Vec3_00406f00(const Vec3_00406f00 &o);              // out of line at 0x406f00
 Vec3_00406f00 &operator=(const Vec3_00406f00 &o);   // folded with the copy ctor
};

struct Mat4E42 {
 float m[16];
 Mat4E42() { Identity_0040cc50(); }
 void Identity_0040cc50();
 Mat4E42 Mul_0040cc90(const Mat4E42 &b) const;
 void Assign_0040cba0(const Mat4E42 &o);
 void RotateX_0041fdd0(float degrees);
 void RotateY_0041fe00(float degrees);
 void RotateZ_0041fe30(float degrees);
 void SetTranslation_00420200(const Vec3_00406f00 &v);
};

// Mesh / transform record referenced by scene objects (+0x38).
struct SceneMesh {
 unsigned reserved0;
 float ox, oy, oz;                 // 0x04 offset (interface objects: screen position)
 unsigned char reserved10[0x2c - 0x10];
 float x, y, z;                    // 0x2c local position
 float width, height, depth;       // 0x38 half size
 float ex, ey, ez;                 // 0x44 half extent
 float radius;                     // 0x50
 unsigned char reserved54[0x94 - 0x54];
 float matrix[16];                 // 0x94 world matrix
};

// Polymorphic scene object (graphic, text window, sound...).
class SceneObject {
public:
 virtual ~SceneObject();            // slot 0: virtual deleting destructor
 virtual int Type();
 virtual void Slot2();
 virtual void Render(int time);    // +0x0c
 virtual void Update(int time);    // +0x10
 unsigned reserved4, reserved8;
 SceneObject *next;
 unsigned reserved10;
 char name[0x34 - 0x14];
 void *shape;                      // 0x34
 SceneMesh *mesh;                  // 0x38
};

// Octree node (count/objects/children/center/half size).
struct SpatialNodeE42 {
 int count;
 SceneObject **objects;
 bool split;
 SpatialNodeE42 *children[8];
 float cx, cy, cz;                 // 0x2c
 float half;                       // 0x38
 ~SpatialNodeE42() {
  count = 0;
  objects = 0;
  if (split) {
   for (int i = 0; i < 8; i++) {
    if (children[i]) delete children[i];
    children[i] = 0;
   }
  }
  split = false;
 }
};

template<class T> struct ScenePtrArray {
 T *data; int size, capacity, grow_by;
 ScenePtrArray() { data = 0; size = capacity = grow_by = 0; }
 ~ScenePtrArray() { if (data) ::operator delete(data); }
};
struct SceneRecord40 { unsigned char raw[0x28]; ~SceneRecord40(); };   // dtor folded at 0x4051b0
struct SceneRecordArray {
 SceneRecord40 *data; int size, capacity, grow_by;
 SceneRecordArray() { data = 0; size = capacity = grow_by = 0; }
 ~SceneRecordArray() {
  if (data) { SceneRecord40 *p = data; for (int n = size; n; n--) { p->~SceneRecord40(); ++p; } ::operator delete(data); }
 }
};

struct SceneMessageArray {
 ObservedPickingMessage *data; int size, capacity, grow_by;
 SceneMessageArray() { data = 0; size = capacity = grow_by = 0; }
 ~SceneMessageArray() { if (data) ::operator delete(data); }
};
struct SceneProjection {
 float m[16];
 float scale, fov;
 SceneProjection() {
  m[0] = m[5] = m[10] = m[15] = 1;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
  scale = 1.0f; fov = 40.0f;
 }
};

void __stdcall ConstructMessages_00426690(ObservedPickingMessage *, int);
void __stdcall DestructMessages_004120c0(ObservedPickingMessage *, int);

extern int g_005026bc;          // scene clock (ms, scaled)
extern float g_005026b8;        // game speed
extern int g_005026c0;          // last frame delta
extern unsigned long g_005026c4, g_005026c8;

class Scene {  // 0x421690 (was Scene_00421690)
public:
 int frozen;                                   // 0x0000 frames left without rendering
 ScenePtrArray<SceneObject *> statics;        // 0x0004
 ScenePtrArray<SceneObject *> objects;        // 0x0014
 ScenePtrArray<SceneObject *> windows;        // 0x0024
 SceneRecordArray records;                     // 0x0034
 SceneObject *visible[1000];                   // 0x0044
 int visibleCount;                             // 0x0fe4
 ScenePairArray pairs;                         // 0x0fe8
 SceneMessageArray messages;                   // 0x0ffc
 ObservedSceneSpatial spatial;                 // 0x100c
 unsigned reserved1060;
 bool flag1064;
 unsigned char reserved1065;
 bool drawOverlay;                             // 0x1066
 unsigned char reserved1067;
 float planes[6][4];                           // 0x1068
 SceneProjection projection;                   // 0x10c8
 bool cameraSet;                               // 0x1110
 void *selected;                               // 0x1114
 SoundFocusPrefix sound;                       // 0x1118
 SceneObject *pointNode;                       // 0x1150 cursor object
 int cursorType;                               // 0x1154

 Scene();
 ~Scene();
 void RemoveAll_00421de0(unsigned flags);
 void Frame_00421b40();
 bool Load_00421a20(const char *name);
 void ExtractFrustum_00421ce0();
 bool CubeVisible_00422060(float x, float y, float z, float size);
 bool BoxVisible_004221e0(const Vec3_00406f00 &center, Vec3_00406f00 extent);
 bool SphereVisible_00422370(float x, float y, float z, float radius);
 void NormalizePlane_004223c0(float p[][4], int index);
 bool ObjectVisible_00422420(SceneObject *object);
 void DrawNode_00423d90(SpatialNodeE42 *node);
 void Drain_00424590();
 bool Send_00424780(ObservedPickingMessage *message);
 void DrawOverlay_00424a30();
 void LoadScene_00422510(ObservedString path);
 void ReadResources_00422a90(struct DomNode42 *node);
 void ReadObjects_00422d40(struct DomNode42 *node, int kind);
 void ReadCamera_00423f80(struct DomNode42 *node);
 void ReadSettings_00425cc0(struct DomNode42 *node);
 void ReadAmbient_00424230(struct DomNode42 *node);
 void ReadLighting_00423e40(struct DomNode42 *node);
 void BuildOctree_00423a80(bool rebuild);
 void RemoveObject_004256d0(SceneObject *object);
 SceneObject *CreateTextWindow_00424bb0(SceneObject *parent, const char *text, int x, int y, int width,
                                        int height, int size, int color, const char *script);
 void CreateTextWindowI_00425640(const char *key, int x, int y, int width, int height,
                                 int a, int b, const char *font);
 void DeleteObject_00425690(SceneObject *object);
 void AddObject_00425860(SceneObject *object, int kind);
 void SetCursorType_00425de0(int type);
 SceneObject *CreateAviPlane_00425a20(float x, float y, float z, float width, float height, int p6,
                                      int p7, int p8, int p9, int p10, int p11, int p12, bool loop,
                                      const char *file, const char *script);
 SceneObject *CreateIFWindow_00425150(SceneObject *parent, Vec3_00406f00 pos, Vec3_00406f00 size,
                                      Vec3_00406f00 uv0, Vec3_00406f00 uv1, const char *texture,
                                      const char *script);
};
extern Scene g_scene;   // see symbols.json
#endif
