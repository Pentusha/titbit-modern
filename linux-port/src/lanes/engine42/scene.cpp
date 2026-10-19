#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Scene: construction, per-frame update/render, frustum culling (0x421690-0x422420, 0x423d90).
#include "e42_scene.h"
extern "C" char *__cdecl strcpy(char *, const char *);
extern "C" unsigned __cdecl strlen(const char *);
#pragma intrinsic(strcpy, strlen)

struct Camera_0041e8f0 {
 void Begin_0041e9f0();
 void Setup_0041f3b0();
 void Apply_0041f3c0(SceneObject *object);
 void End_0041ea00();
};
extern Camera_0041e8f0 g_005026d0;
void __cdecl ResetView_0041ad30();
void __cdecl FUN_00402c60();
void __cdecl UpdateSounds_0042a0b0();

// NONMATCHING: faithful translation (message array SetSize inlined)
Scene::Scene() {
 flag1064 = false;
 ResetView_0041ad30();
 SceneMessageArray &q = messages;
 q.grow_by = 1000;
 if (q.data == 0) {
  q.data = (ObservedPickingMessage *)::operator new(1000 * sizeof(ObservedPickingMessage));
  ConstructMessages_00426690(q.data, 1000);
  q.capacity = 1000;
  q.size = 1000;
 } else if (q.capacity >= 1000) {
  if (q.size < 1000) ConstructMessages_00426690(&q.data[q.size], 1000 - q.size);
  else if (q.size > 1000) DestructMessages_004120c0(&q.data[1000], q.size - 1000);
  q.size = 1000;
 } else {
  int newCapacity = q.capacity + 1000;
  if (newCapacity <= 1000) newCapacity = 1000;
  ObservedPickingMessage *data = (ObservedPickingMessage *)::operator new(newCapacity * sizeof(ObservedPickingMessage));
  memcpy(data, q.data, q.size * sizeof(ObservedPickingMessage));
  if (q.size < 1000) ConstructMessages_00426690(&data[q.size], 1000 - q.size);
  ::operator delete(q.data);
  q.data = data;
  q.size = 1000;
  q.capacity = newCapacity;
 }
 drawOverlay = true;
 selected = 0;
 pointNode = 0;
 frozen = 0;
 g_005026bc = 0;
 g_005026b8 = 1.0f;
}

// NONMATCHING: faithful translation
Scene::~Scene() {
 RemoveAll_00421de0(0xff);
}

// NONMATCHING: faithful translation
void Scene::Frame_00421b40() {
 FUN_00402c60();
 UpdateSounds_0042a0b0();
 unsigned long now = timeGetTime();
 g_005026c8 = now;
 g_005026c0 = (int)((float)(now - g_005026c4) * g_005026b8);
 g_005026c4 = now;
 g_005026bc += g_005026c0;
 ExtractFrustum_00421ce0();
 g_005026d0.Begin_0041e9f0();
 g_005026d0.Setup_0041f3b0();
 Drain_00424590();
 visibleCount = 0;
 DrawNode_00423d90((SpatialNodeE42 *)&spatial.root);
 glEnable(0xb50);                                  // GL_LIGHTING
 int count = objects.size;
 for (int i = 0; i < count; i++) {
  SceneObject *object = objects.data[i];
  if (object->mesh) {
   object->Update(g_005026bc);
   if (ObjectVisible_00422420(object)) visible[visibleCount++] = object;
  }
 }
 if (frozen == 0) {
  for (int i = 0; i < visibleCount; i++) {
   glPushMatrix();
   g_005026d0.Apply_0041f3c0(visible[i]);
   visible[i]->Render(g_005026bc);
   glPopMatrix();
  }
  glDisable(0xb60);                                // GL_FOG
  glDisable(0xb50);                                // GL_LIGHTING
  glLoadIdentity();
  glDisable(0xde1);                                // GL_TEXTURE_2D
  glDisable(0xbc0);                                // GL_ALPHA_TEST
  glDisable(0xbe2);                                // GL_BLEND
  g_005026d0.End_0041ea00();
  if (drawOverlay) DrawOverlay_00424a30();
 }
 if (frozen > 0) frozen--;
}

// NONMATCHING: faithful translation
void Scene::ExtractFrustum_00421ce0() {
 Mat4E42 modelview;
 Mat4E42 clip;
 glGetFloatv(0xba7, clip.m);                       // GL_PROJECTION_MATRIX
 glGetFloatv(0xba6, modelview.m);                  // GL_MODELVIEW_MATRIX
 clip = clip.Mul_0040cc90(modelview);
 float *v = clip.m;
 for (int i = 0; i < 6; i += 2) {
  planes[i][0] = clip.m[3] - v[0];
  planes[i][1] = clip.m[7] - v[4];
  planes[i][2] = clip.m[11] - v[8];
  planes[i][3] = clip.m[15] - v[12];
  NormalizePlane_004223c0(planes, i);
  planes[i + 1][0] = clip.m[3] + v[0];
  planes[i + 1][1] = clip.m[7] + v[4];
  planes[i + 1][2] = clip.m[11] + v[8];
  planes[i + 1][3] = clip.m[15] + v[12];
  NormalizePlane_004223c0(planes, i + 1);
  ++v;
 }
}

// Cube (center, half size) against the six frustum planes.
// NONMATCHING: faithful translation
bool Scene::CubeVisible_00422060(float x, float y, float z, float size) {
 for (int i = 0; i < 6; i++) {
  float *p = planes[i];
  if (p[0] * (x - size) + p[1] * (y - size) + p[2] * (z - size) + p[3] > 0) continue;
  if (p[0] * (x + size) + p[1] * (y - size) + p[2] * (z - size) + p[3] > 0) continue;
  if (p[0] * (x - size) + p[1] * (y + size) + p[2] * (z - size) + p[3] > 0) continue;
  if (p[0] * (x + size) + p[1] * (y + size) + p[2] * (z - size) + p[3] > 0) continue;
  if (p[0] * (x - size) + p[1] * (y - size) + p[2] * (z + size) + p[3] > 0) continue;
  if (p[0] * (x + size) + p[1] * (y - size) + p[2] * (z + size) + p[3] > 0) continue;
  if (p[0] * (x - size) + p[1] * (y + size) + p[2] * (z + size) + p[3] > 0) continue;
  if (p[0] * (x + size) + p[1] * (y + size) + p[2] * (z + size) + p[3] > 0) continue;
  return false;
 }
 return true;
}

// NONMATCHING: faithful translation
bool Scene::ObjectVisible_00422420(SceneObject *object) {
 SceneMesh *mesh = object->mesh;
 Vec3_00406f00 center;
 center.x = mesh->matrix[4] * mesh->y + mesh->matrix[8] * mesh->z + mesh->matrix[0] * mesh->x + mesh->matrix[12];
 center.y = mesh->matrix[5] * mesh->y + mesh->matrix[9] * mesh->z + mesh->matrix[1] * mesh->x + mesh->matrix[13];
 center.z = mesh->matrix[6] * mesh->y + mesh->matrix[10] * mesh->z + mesh->matrix[2] * mesh->x + mesh->matrix[14];
 if (!SphereVisible_00422370(center.x, center.y, center.z, mesh->radius)) return false;
 return BoxVisible_004221e0(center, Vec3_00406f00(mesh->ex, mesh->ey, mesh->ez));
}

// Recursive octree walk: update + render visible objects of each visible node.
// NONMATCHING: faithful translation
void Scene::DrawNode_00423d90(SpatialNodeE42 *node) {
 if (!CubeVisible_00422060(node->cx, node->cy, node->cz, node->half)) return;
 int count = node->count;
 for (int i = 0; i < count; i++) {
  SceneObject *object = node->objects[i];
  if (object->mesh && ObjectVisible_00422420(object)) {
   object->Update(g_005026bc);
   if (frozen == 0) object->Render(g_005026bc);
  }
 }
 if (node->split)
  for (int k = 0; k < 8; k++)
   if (node->children[k]) DrawNode_00423d90(node->children[k]);
}

// Script SCENE.Load: places the inventory panel and posts a load request.
// NONMATCHING: faithful translation
bool Scene::Load_00421a20(const char *name) {
 Vec3_00406f00 pos(0, 0, -200), size(1024, 768, 0), a(0.5825f, 0.24f, 0), b(0.0375f, -0.05f, 0);
 CreateIFWindow_00425150(0, pos, size, a, b, "items.tga", "suicide.scv");
 char *copy = (char *)::operator new(strlen(name) + 1);
 strcpy(copy, name);
 ObservedPickingMessage message;
 memset(&message, 0, sizeof(message));
 message.event = 0x800;
 message.selected = copy;
 Send_00424780(&message);
 return 1;
}
