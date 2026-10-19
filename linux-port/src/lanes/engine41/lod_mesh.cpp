// LOD mesh drawing: skinning into per-object vertex buffers (0x415190), buffer
// setup with bounding volume (0x4150a0) and the GL draw passes (0x4147d0).
#include <math.h>
#include "e41_string.h"
#include "lane_xml.h"
void __cdecl PickingTrace(const char *, ...);               // engine log, 0x403650
extern "C" {
__declspec(dllimport) void __stdcall glColor4f(float, float, float, float);
__declspec(dllimport) void __stdcall glEnable(unsigned);
__declspec(dllimport) void __stdcall glDisable(unsigned);
__declspec(dllimport) void __stdcall glBlendFunc(unsigned, unsigned);
__declspec(dllimport) void __stdcall glBegin(unsigned);
__declspec(dllimport) void __stdcall glEnd(void);
__declspec(dllimport) void __stdcall glBindTexture(unsigned, unsigned);
__declspec(dllimport) void __stdcall glTexCoord2f(float, float);
__declspec(dllimport) void __stdcall glNormal3fv(const float *);
__declspec(dllimport) void __stdcall glVertex3fv(const float *);
__declspec(dllimport) void __stdcall glDepthMask(unsigned char);
__declspec(dllimport) void __stdcall glDepthFunc(unsigned);
__declspec(dllimport) void __stdcall glGetFloatv(unsigned, float *);
}
#pragma intrinsic(sqrt)

struct Vec3L { float x, y, z; Vec3L() { x = y = z = 0; } };
struct Mat4L {
 float m[16];
 Mat4L() { Identity(); }
 void Identity();                     // 0x40cc50
};

// Mesh record of the global mesh cache (0x78 bytes).
struct MeshRecord {
 char name[0x2c];
 int vertex_count;                    // +0x2c
 float *vertices;                     // +0x30 (xyz)
 int unknown34;
 int *vertex_bones;                   // +0x38
 int triangle_count;                  // +0x3c
 int *triangles;                      // +0x40 (3 indices)
 int *materials;                      // +0x44
 unsigned flags;                      // +0x48 (2 = second texture layer)
 float *uv;                           // +0x4c (6 per triangle)
 int unknown50;
 float *uv2;                          // +0x54
 int first_triangle;                  // +0x58
 int transparent_count;               // +0x5c (last ones, sorted by the renderer)
 int skin_count;                      // +0x60
 char has_normals;                    // +0x64 (also: environment pass)
 float *normals;                      // +0x68
 char tail[0x78 - 0x6c];
};
extern "C" int g_005024a4;
extern "C" MeshRecord *g_005024a8;

struct MaterialSlot { char head[0x60]; unsigned texture, texture2; int pad; char two_sided; char pad6d[3]; unsigned env_texture; };
struct ResourceCache { MaterialSlot *Get_00417270(int index); }; // original global 0x5ee6dc
extern ResourceCache g_005ee6dc;

struct TransparentList_005026d0 {
 void Add_0041ef40(const float *a, const float *b, const float *c, const float *uv, unsigned *texture);
};
extern TransparentList_005026d0 g_005026d0;

struct BoneL { char head[0x2c]; float center[3]; float extents[3]; char pad44[0x0c]; float radius; char pad54[0x94 - 0x54]; float world[16]; char tail[0xd8 - 0xd4]; };
struct SkeletonL { char head[0x34]; int bone_count; BoneL *bones; };
struct AnimationCache { int Find_00415b40(ObservedString name); int Load_004158c0(ObservedString path, int flags); }; // original global 0x5024a0
extern AnimationCache g_005024a0;
void __cdecl LoadMeat_00418cd0(ObservedString file, int *count, int **indices);

struct XmlRef : XmlElement_v { void Release_00415890(); };
void XmlRef::Release_00415890() { if (--refs <= 0) Destroy(1); }

struct OwnerL { char head[0x38]; BoneL *bone; };

class Lod { // original 0x414150
public:
 void *vtable;
 OwnerL *owner;                       // +0x04
 char body[0xd4 - 8];
 float near_distance, far_distance;   // +0xd4
 int meat;                            // +0xdc
 Vec3L *skinned;                      // +0xe0
 Vec3L *skinned_normals;              // +0xe4
 MeshRecord *Meat() const { return (meat >= 0 && meat < g_005024a4) ? &g_005024a8[meat] : 0; }
 void DrawMesh_004147d0();
 void ComputeBounds_00414530();                 // tail-called by PrepareBuffers
 void PrepareBuffers_004150a0();
 void Skin_00415190();
 bool Load_004153b0(XmlNode_v *node);
};

void Lod::DrawMesh_004147d0() {
 int material = -1;
 if (meat < 0 || meat >= g_005024a4) return;
 MeshRecord *mesh = &g_005024a8[meat];
 if (!mesh) return;
 MaterialSlot *slot;
 const float *last;
 glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
 glEnable(0xbc0);                     // GL_ALPHA_TEST
 glEnable(0xbe2);                     // GL_BLEND
 glBlendFunc(0x302, 0x303);
 glEnable(0xde1);                     // GL_TEXTURE_2D
 glBegin(4);
 int i;
 for (i = mesh->first_triangle; i < mesh->triangle_count - mesh->transparent_count; i++) {
  int *tri = &mesh->triangles[i * 3];
  if (material != mesh->materials[i]) {
   glEnd();
   material = mesh->materials[i];
   slot = g_005ee6dc.Get_00417270(material);
   glBindTexture(0xde1, slot->texture);
   if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);   // GL_CULL_FACE
   glBegin(4);
  }
  float *uv = &mesh->uv[i * 6];
  glTexCoord2f(uv[0], uv[1]);
  glNormal3fv(&skinned_normals[tri[0]].x);
  glVertex3fv(&skinned[tri[0]].x);
  glTexCoord2f(mesh->uv[i * 6 + 2], mesh->uv[i * 6 + 3]);
  glNormal3fv(&skinned_normals[tri[1]].x);
  glVertex3fv(&skinned[tri[1]].x);
  glTexCoord2f(mesh->uv[i * 6 + 4], mesh->uv[i * 6 + 5]);
  last = &skinned[tri[2]].x;
  glNormal3fv(&skinned_normals[tri[2]].x);
  glVertex3fv(last);
 }
 glEnd();
 if (mesh->flags & 2) {
  material = -1;
  glDepthMask(0);
  glDepthFunc(0x202);                 // GL_EQUAL
  glBlendFunc(0, 0x300);
  glBegin(4);
  for (i = mesh->first_triangle; i < mesh->triangle_count - mesh->transparent_count; i++) {
   int *tri = &mesh->triangles[i * 3];
   if (material != mesh->materials[i]) {
    glEnd();
    material = mesh->materials[i];
    slot = g_005ee6dc.Get_00417270(material);
    glBindTexture(0xde1, slot->texture2);
    if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
    glBegin(4);
   }
   float *uv = &mesh->uv2[i * 6];
   glTexCoord2f(uv[0], uv[1]);
   glVertex3fv(last);                 // sic: the previous triangle's last vertex
   glTexCoord2f(mesh->uv2[i * 6 + 2], mesh->uv2[i * 6 + 3]);
   glVertex3fv(&skinned[tri[1]].x);
   glTexCoord2f(mesh->uv2[i * 6 + 4], mesh->uv2[i * 6 + 5]);
   last = &skinned[tri[2]].x;
   glVertex3fv(last);
  }
  glEnd();
  glDepthMask(1);
  glDepthFunc(0x203);                 // GL_LEQUAL
 }
 if (mesh->has_normals) {
  material = -1;
  glEnable(0xc60);                    // GL_TEXTURE_GEN_S
  glEnable(0xc61);                    // GL_TEXTURE_GEN_T
  glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
  glDepthMask(0);
  glBlendFunc(0x306, 0x308);
  glBegin(4);
  for (i = mesh->first_triangle; i < mesh->triangle_count - mesh->transparent_count; i++) {
   if (material != mesh->materials[i]) {
    glEnd();
    material = mesh->materials[i];
    slot = g_005ee6dc.Get_00417270(material);
    if (slot->env_texture) glBindTexture(0xde1, slot->env_texture);
    if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
    glBegin(4);
   }
   if (slot->env_texture) {
    int *tri = &mesh->triangles[i * 3];
    const float *v0 = &skinned[tri[0]].x, *v1 = &skinned[tri[1]].x, *v2 = &skinned[tri[2]].x;
    const float *n2 = &skinned_normals[tri[2]].x;
    glNormal3fv(&skinned_normals[tri[0]].x);
    glVertex3fv(v0);
    glNormal3fv(&skinned_normals[tri[1]].x);
    glVertex3fv(v1);
    glNormal3fv(n2);
    glVertex3fv(v2);
   }
  }
  glEnd();
  glDepthMask(1);
  glBlendFunc(0x302, 0x303);
  glDisable(0xc60);
  glDisable(0xc61);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
 }
 glDisable(0xde1);
 glDisable(0xbc0);
 glDisable(0xbe2);
 // Transparent triangles go to the renderer's depth-sorted list in eye space.
 for (i = mesh->triangle_count - mesh->transparent_count; i < mesh->triangle_count; i++) {
  Mat4L m;
  m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
  m.m[1] = m.m[2] = m.m[3] = m.m[4] = m.m[6] = m.m[7] = m.m[8] = m.m[9] = m.m[11] = m.m[12] = m.m[13] = m.m[14] = 0;
  glGetFloatv(0xba6, m.m);            // GL_MODELVIEW_MATRIX
  int *tri = &mesh->triangles[i * 3];
  unsigned texture = g_005ee6dc.Get_00417270(mesh->materials[i])->texture;
  float a[3], b[3], c[3];
  const Vec3L &p2 = skinned[tri[2]];
  c[2] = m.m[2] * p2.x + m.m[6] * p2.y + m.m[10] * p2.z + m.m[14];
  c[1] = m.m[1] * p2.x + m.m[5] * p2.y + m.m[9] * p2.z + m.m[13];
  c[0] = m.m[0] * p2.x + m.m[4] * p2.y + m.m[8] * p2.z + m.m[12];
  const Vec3L &p1 = skinned[tri[1]];
  b[2] = m.m[6] * p1.y + m.m[10] * p1.z + m.m[2] * p1.x + m.m[14];
  b[1] = m.m[5] * p1.y + m.m[9] * p1.z + m.m[1] * p1.x + m.m[13];
  const Vec3L &p0 = skinned[tri[0]];
  b[0] = m.m[4] * p1.y + m.m[8] * p1.z + m.m[0] * p1.x + m.m[12];
  a[2] = m.m[6] * p0.y + m.m[10] * p0.z + m.m[2] * p0.x + m.m[14];
  a[1] = m.m[5] * p0.y + m.m[9] * p0.z + m.m[1] * p0.x + m.m[13];
  a[0] = m.m[0] * p0.x + m.m[4] * p0.y + m.m[8] * p0.z + m.m[12];
  g_005026d0.Add_0041ef40(a, b, c, &mesh->uv[i * 6], &texture);
 }
}

// Allocate skinning buffers; on first use derive the owner bone's bounding
// box centre, half extents and radius from the mesh vertices.
// Bounding box/sphere of the mesh when the bone has none yet (0x414530).
void Lod::ComputeBounds_00414530() {
 BoneL *bone = owner->bone;
 if (bone->extents[0] == 0.0f && bone->extents[1] == 0.0f && bone->extents[2] == 0.0f) {
  MeshRecord *m = Meat();
  float *v = m->vertices;
  float maxx = v[0], maxy = v[1], maxz = v[2];
  float minx = maxx, miny = maxy, minz = maxz;
  for (int n = m->vertex_count - 1; n > 0; n--) {
   v += 3;
   if (v[0] < maxx) { if (v[0] < minx) minx = v[0]; } else maxx = v[0];
   if (v[1] < maxy) { if (v[1] < miny) miny = v[1]; } else maxy = v[1];
   if (v[2] < maxz) { if (v[2] < minz) minz = v[2]; } else maxz = v[2];
  }
  float hx = (maxx - minx) * 0.5f;
  owner->bone->center[0] = hx + minx;
  float hy = (maxy - miny) * 0.5f;
  owner->bone->center[1] = hy + miny;
  float hz = (maxz - minz) * 0.5f;
  owner->bone->center[2] = hz + minz;
  float radius = 0.0f;
  owner->bone->extents[0] = hx;
  owner->bone->extents[1] = hy;
  owner->bone->extents[2] = hz;
  float *p = m->vertices;
  BoneL *b = owner->bone;
  for (int k = m->vertex_count; k; k--, p += 3) {
   float dx = b->center[0] - p[0], dy = b->center[1] - p[1], dz = b->center[2] - p[2];
   float d = (float)sqrt(dx * dx + dy * dy + dz * dz);
   if (radius < d) radius = d;
  }
  owner->bone->radius = radius;
 }
}


void Lod::PrepareBuffers_004150a0() {
 if (skinned) operator delete(skinned);
 if (skinned_normals) operator delete(skinned_normals);
 MeshRecord *mesh = Meat();
 if (mesh->skin_count) {
  skinned = new Vec3L[mesh->skin_count];
  skinned_normals = new Vec3L[mesh->skin_count];
 }
 ComputeBounds_00414530();
}

// Transform the mesh vertices (and normals) by their bones into the buffers.
void Lod::Skin_00415190() {
 Mat4L rotation;
 MeshRecord *mesh = Meat();
 int bone = -1;
 const float *m = (const float *)this;
 for (int i = 0; i < mesh->skin_count; i++) {
  int b = mesh->vertex_bones[i];
  if (b != bone) {
   bone = b;
   SkeletonL *skeleton = (SkeletonL *)owner;
   if (b < 0 || b >= skeleton->bone_count) continue;
   m = skeleton->bones[b].world;
   if (mesh->has_normals) {
    for (int k = 0; k < 16; k++) rotation.m[k] = m[k];
    rotation.m[12] = rotation.m[13] = rotation.m[14] = 0;
   }
  }
  const float *v = &mesh->vertices[i * 3];
  Vec3L &out = skinned[i];
  out.x = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
  out.z = m[6] * v[1] + m[10] * v[2] + m[2] * v[0] + m[14];
  out.y = m[5] * v[1] + m[9] * v[2] + m[1] * v[0] + m[13];
  if (mesh->has_normals) {
   const float *n = &mesh->normals[i * 3];
   Vec3L &o = skinned_normals[i];
   o.x = rotation.m[0] * n[0] + rotation.m[4] * n[1] + rotation.m[8] * n[2] + rotation.m[12];
   o.z = rotation.m[2] * n[0] + rotation.m[6] * n[1] + rotation.m[10] * n[2] + rotation.m[14];
   o.y = rotation.m[1] * n[0] + rotation.m[5] * n[1] + rotation.m[9] * n[2] + rotation.m[13];
   float length = (float)sqrt(o.z * o.z + o.y * o.y + o.x * o.x);
   if (length != 0.0f) {
    length = 1.0f / length;
    o.x = length * o.x;
    o.y = length * o.y;
    o.z = length * o.z;
   }
  }
 }
}

bool Lod::Load_004153b0(XmlNode_v *node) {
 PickingTrace("Start with LOD");
 XmlRef *element = (XmlRef *)node->ToElement();
 XmlAttr_v *a = element->Attribute(std::string("fardistance"));
 if (a) far_distance = (float)atof(a->Value()->c_str());
 else far_distance = 10000.0f;
 a = element->Attribute(std::string("neardistance"));
 if (a) near_distance = (float)atof(a->Value()->c_str());
 else near_distance = 0.0f;
 a = element->Attribute(std::string("meatname"));
 if (!a) {
  element->Release_00415890();
  PickingTrace("Failed to find meatname section");
  return false;
 }
 int *indices = 0;
 int count;
 ObservedString file(a->Value()->c_str());
 PickingTrace(file);
 file.Remove(' ');
 ObservedString name = file.Left(file.ReverseFind('.'));
 int index = g_005024a0.Find_00415b40(name);
 if (index != -1) index = g_005024a0.Load_004158c0(file, -1);
 else {
  LoadMeat_00418cd0(file, &count, &indices);
  if (count < 1 || !indices) {
   PickingTrace("Failed to create LOD");
   return false;
  }
  index = indices[0];
 }
 meat = index;
 PrepareBuffers_004150a0();
 element->Release_00415890();
 PickingTrace("LOD loaded");
 return true;
}
