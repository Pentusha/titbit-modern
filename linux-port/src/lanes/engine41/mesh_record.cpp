// Mesh ("meat") record methods: triangle sort by material, normals, display lists.
#include <math.h>
#include "e41_string.h"
void __cdecl PickingTrace(const char *, ...);               // engine log, 0x403650
extern "C" {
void *__cdecl memset(void *, int, unsigned);
__declspec(dllimport) void __stdcall glColor4f(float, float, float, float);
__declspec(dllimport) void __stdcall glEnable(unsigned);
__declspec(dllimport) void __stdcall glDisable(unsigned);
__declspec(dllimport) void __stdcall glBlendFunc(unsigned, unsigned);
__declspec(dllimport) void __stdcall glBegin(unsigned);
__declspec(dllimport) void __stdcall glEnd(void);
__declspec(dllimport) void __stdcall glBindTexture(unsigned, unsigned);
__declspec(dllimport) void __stdcall glTexCoord2f(float, float);
__declspec(dllimport) void __stdcall glNormal3f(float, float, float);
__declspec(dllimport) void __stdcall glVertex3fv(const float *);
__declspec(dllimport) void __stdcall glDepthMask(unsigned char);
__declspec(dllimport) void __stdcall glDepthFunc(unsigned);
__declspec(dllimport) unsigned __stdcall glGenLists(int);
__declspec(dllimport) void __stdcall glNewList(unsigned, unsigned);
__declspec(dllimport) void __stdcall glEndList(void);
}
#pragma intrinsic(sqrt, memset)

struct Vec3M { float x, y, z; Vec3M() { x = y = z = 0; } float Length_00406f50() const; };
struct TriM { int a, b, c; };
struct UvM { float uv[6]; };

struct MaterialSlot { char head[0x5c]; int blend; unsigned texture, texture2, texture3; char two_sided; char pad[3]; unsigned env_texture; };
struct ResourceCache { MaterialSlot *Get_00417270(int index); int Count_00417920(); int Load_00417390(ObservedString &file, int flags, int options); }; // original global 0x5ee6dc
extern ResourceCache g_005ee6dc;
extern "C" {
extern char g_00502660;                                              // multitexture available
extern void (__stdcall *g_00502658)(unsigned texture);               // glActiveTextureARB
extern void (__stdcall *g_0050264c)(unsigned texture, float s, float t);   // glMultiTexCoord2fARB
}

struct MeshRecord_00417940 {
 char name[0x20];
 int used, refs, age;                 // +0x20 cache slot state
 int vertex_count;                    // +0x2c
 Vec3M *vertices;                     // +0x30
 int part_count;                      // +0x34
 int *vertex_bones;                   // +0x38
 int triangle_count;                  // +0x3c
 TriM *triangles;                     // +0x40
 int *materials;                      // +0x44
 unsigned flags;                      // +0x48: 1 uv, 2 uv2, 4 uv3
 UvM *uv;                             // +0x4c
 UvM *uv3;                            // +0x50
 UvM *uv2;                            // +0x54
 int first_triangle, transparent_count, skin_count;   // +0x58
 char has_normals;                    // +0x64
 Vec3M *normals;                      // +0x68
 unsigned *lists;                     // +0x6c
 int *resources;                      // +0x70 material slots
 int resource_count;                  // +0x74
 void SortTriangles_00417a90(int *keys, int lo, int hi);
 void ComputeNormals_00418060();
 void BuildLists_004182d0();
 void DrawPart_00418350(int part);
 void Partition_00417c90();
};

// Quicksort of the triangles (and their per-triangle data) by key.
void MeshRecord_00417940::SortTriangles_00417a90(int *keys, int lo, int hi) {
 int i, j;
 do {
  int pivot = keys[(lo + hi) / 2];
  i = lo; j = hi;
  do {
   while (keys[i] < pivot && i < hi) i++;
   while (pivot < keys[j] && lo < j) j--;
   if (j < i) break;
   TriM t = triangles[i]; triangles[i] = triangles[j]; triangles[j] = t;
   int k = keys[i]; keys[i] = keys[j]; keys[j] = k;
   if (flags & 1) { UvM u = uv[i]; uv[i] = uv[j]; uv[j] = u; }
   if (flags & 2) { UvM u = uv2[i]; uv2[i] = uv2[j]; uv2[j] = u; }
   if (flags & 4) { UvM u = uv3[i]; uv3[i] = uv3[j]; uv3[j] = u; }
   if (materials) { int m = materials[i]; materials[i] = materials[j]; materials[j] = m; }
   i++;
   j--;
  } while (i <= j);
  if (lo < j) SortTriangles_00417a90(keys, lo, j);
  lo = i;
 } while (i < hi);
}

// Smooth vertex normals from the face normals (once).
void MeshRecord_00417940::ComputeNormals_00418060() {
 if (normals) return;
 normals = new Vec3M[vertex_count];
 has_normals = 1;
 int i;
 for (i = 0; i < vertex_count; i++) { normals[i].x = 0; normals[i].y = 0; normals[i].z = 0; }
 for (i = 0; i < triangle_count; i++) {
  TriM &t = triangles[i];
  Vec3M &p = vertices[t.a];
  float ex = vertices[t.c].x - p.x, ey = vertices[t.c].y - p.y, ez = vertices[t.c].z - p.z;
  float fx = vertices[t.b].x - p.x, fy = vertices[t.b].y - p.y, fz = vertices[t.b].z - p.z;
  float nx = fy * ez - fz * ey;
  float ny = fz * ex - ez * fx;
  float nz = ey * fx - fy * ex;
  float length = (float)sqrt(nx * nx + ny * ny + nz * nz);
  if (length != 0.0f) { length = 1.0f / length; nx = nx * length; ny = ny * length; nz = nz * length; }
  Vec3M &n0 = normals[t.a]; n0.x = nx + n0.x; n0.y = ny + n0.y; n0.z = nz + n0.z;
  Vec3M &n1 = normals[t.b]; n1.x = nx + n1.x; n1.y = ny + n1.y; n1.z = nz + n1.z;
  Vec3M &n2 = normals[t.c]; n2.x = nx + n2.x; n2.y = ny + n2.y; n2.z = nz + n2.z;
 }
 for (i = 0; i < vertex_count; i++) {
  Vec3M &n = normals[i];
  float length = n.Length_00406f50();
  if (length != 0.0f) { length = 1.0f / length; n.x = length * n.x; n.y = length * n.y; n.z = length * n.z; }
 }
}

void MeshRecord_00417940::BuildLists_004182d0() {
 unsigned *ids = lists;
 unsigned base;
 if (!ids) {
  ids = (unsigned *)operator new(part_count * 4);
  lists = ids;
  base = glGenLists(part_count);
 } else base = ids[0];
 for (int i = 0; i < part_count; i++) {
  glNewList(i + base, 0x1300);        // GL_COMPILE
  DrawPart_00418350(i);
  glEndList();
  ids[i] = i + base;
 }
}

// Draw the triangles whose first vertex belongs to bone `part` (display list body).
void MeshRecord_00417940::DrawPart_00418350(int part) {
 int material = -1;
 if (!materials) return;
 MaterialSlot *slot;
 int i;
 glEnable(0xbe2);
 glEnable(0xde1);
 glBlendFunc(0x302, 0x303);
 if (!g_00502660 || !(flags & 1)) {
  glEnable(0xde1);
  glDisable(0xbe2);
  glBegin(4);
  if (flags & 1) {
   for (i = 0; i < first_triangle; i++) {
    TriM *t = &triangles[i];
    if (vertex_bones[t->a] != part) continue;
    if (material != materials[i]) {
     glEnd();
     material = materials[i];
     slot = g_005ee6dc.Get_00417270(material);
     glBindTexture(0xde1, slot->texture);
     if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
     glBegin(4);
    }
    glTexCoord2f(uv[i].uv[0], uv[i].uv[1]);
    glVertex3fv(&vertices[t->a].x);
    glTexCoord2f(uv[i].uv[2], uv[i].uv[3]);
    glVertex3fv(&vertices[t->b].x);
    glTexCoord2f(uv[i].uv[4], uv[i].uv[5]);
    glVertex3fv(&vertices[t->c].x);
   }
   glEnd();
  }
  if (flags & 2) {
   glEnable(0xbe2);
   material = -1;
   glDepthMask(0);
   glDepthFunc(0x202);
   glBlendFunc(0, 0x300);
   glBegin(4);
   for (i = 0; i < first_triangle; i++) {
    TriM *t = &triangles[i];
    if (vertex_bones[t->a] != part) continue;
    if (material != materials[i]) {
     glEnd();
     material = materials[i];
     slot = g_005ee6dc.Get_00417270(material);
     glBindTexture(0xde1, slot->texture2);
     if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
     glBegin(4);
    }
    glTexCoord2f(uv2[i].uv[0], uv2[i].uv[1]);
    glVertex3fv(&vertices[t->a].x);
    glTexCoord2f(uv2[i].uv[2], uv2[i].uv[3]);
    glVertex3fv(&vertices[t->b].x);
    glTexCoord2f(uv2[i].uv[4], uv2[i].uv[5]);
    glVertex3fv(&vertices[t->c].x);
   }
   glEnd();
   glDepthMask(1);
   glDepthFunc(0x203);
  }
 } else {
  glBegin(4);
  for (i = 0; i < first_triangle; i++) {
   TriM *t = &triangles[i];
   if (vertex_bones[t->a] != part) continue;
   if (material != materials[i]) {
    glEnd();
    material = materials[i];
    slot = g_005ee6dc.Get_00417270(material);
    if (slot->blend == 0) glDisable(0xbe2); else glEnable(0xbe2);
    if (flags & 1) { g_00502658(0x84c0); glEnable(0xde1); glBindTexture(0xde1, slot->texture); }
    if (flags & 2) { g_00502658(0x84c1); glEnable(0xde1); glBindTexture(0xde1, slot->texture2); }
    if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
    glBegin(4);
   }
   if (flags & 1) g_0050264c(0x84c0, uv[i].uv[0], uv[i].uv[1]);
   if (flags & 2) g_0050264c(0x84c1, uv2[i].uv[0], uv2[i].uv[1]);
   glNormal3f(normals[t->a].x, normals[t->a].y, normals[t->a].z);
   glVertex3fv(&vertices[t->a].x);
   if (flags & 1) g_0050264c(0x84c0, uv[i].uv[2], uv[i].uv[3]);
   if (flags & 2) g_0050264c(0x84c1, uv2[i].uv[2], uv2[i].uv[3]);
   glNormal3f(normals[t->b].x, normals[t->b].y, normals[t->b].z);
   glVertex3fv(&vertices[t->b].x);
   if (flags & 1) g_0050264c(0x84c0, uv[i].uv[4], uv[i].uv[5]);
   if (flags & 2) g_0050264c(0x84c1, uv2[i].uv[4], uv2[i].uv[5]);
   glNormal3f(normals[t->c].x, normals[t->c].y, normals[t->c].z);
   glVertex3fv(&vertices[t->c].x);
  }
  glEnd();
  if (flags & 2) { g_00502658(0x84c1); glDisable(0xde1); }
  if (flags & 1) { g_00502658(0x84c0); glDisable(0xde1); }
 }
 glEnable(0xde1);
 if (flags & 4) {
  glEnable(0xbe2);
  material = -1;
  glDepthMask(0);
  glDepthFunc(0x202);
  glBlendFunc(0, 0x300);
  glBegin(4);
  for (i = 0; i < first_triangle; i++) {
   TriM *t = &triangles[i];
   if (vertex_bones[t->a] != part) continue;
   if (material != materials[i]) {
    glEnd();
    material = materials[i];
    slot = g_005ee6dc.Get_00417270(material);
    glBindTexture(0xde1, slot->texture3);
    if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
    glBegin(4);
   }
   glTexCoord2f(uv3[i].uv[0], uv3[i].uv[1]);
   glVertex3fv(&vertices[t->a].x);
   glTexCoord2f(uv3[i].uv[2], uv3[i].uv[3]);
   glVertex3fv(&vertices[t->b].x);
   glTexCoord2f(uv3[i].uv[4], uv3[i].uv[5]);
   glVertex3fv(&vertices[t->c].x);
  }
  glEnd();
  glDepthMask(1);
  glDepthFunc(0x203);
 }
 if (has_normals) {
  material = -1;
  glEnable(0xbe2);
  glEnable(0xde1);
  glEnable(0xc60);
  glEnable(0xc61);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glDepthMask(0);
  glBlendFunc(0x306, 0x305);
  glBegin(4);
  for (i = 0; i < first_triangle; i++) {
   TriM *t = &triangles[i];
   if (vertex_bones[t->a] != part) continue;
   if (material != materials[i]) {
    glEnd();
    material = materials[i];
    slot = g_005ee6dc.Get_00417270(material);
    if (slot->env_texture) glBindTexture(0xde1, slot->env_texture);
    if (!slot->two_sided) glEnable(0xb44); else glDisable(0xb44);
    glBegin(4);
   }
   if (slot->env_texture) {
    glNormal3f(normals[t->a].x, normals[t->a].y, normals[t->a].z);
    glVertex3fv(&vertices[t->a].x);
    glNormal3f(normals[t->b].x, normals[t->b].y, normals[t->b].z);
    glVertex3fv(&vertices[t->b].x);
    glNormal3f(normals[t->c].x, normals[t->c].y, normals[t->c].z);
    glVertex3fv(&vertices[t->c].x);
   }
  }
  glEnd();
  glDepthMask(1);
  glBlendFunc(0x302, 0x303);
  glDisable(0xc60);
  glDisable(0xc61);
 }
 glDisable(0xde1);
 glDisable(0xbe2);
}

// Order triangles by (transparent, multi-bone, material) and vertices so that
// software-skinned ones come first.
void MeshRecord_00417940::Partition_00417c90() {
 ObservedString message;
 int *remap = (int *)operator new(vertex_count * 4);
 int *keys = (int *)operator new(triangle_count * 4);
 memset(remap, 0, vertex_count * 4);
 int rigid = 0;
 int transparent = 0;
 int material_count = g_005ee6dc.Count_00417920();
 int i;
 for (i = 0; i < triangle_count; i++) {
  int key = 0;
  if (materials && g_005ee6dc.Get_00417270(materials[i])->blend) {
   key = triangle_count * 2;
   transparent++;
  }
  TriM &t = triangles[i];
  int bone = vertex_bones[t.b];
  if (vertex_bones[t.a] != bone || bone != vertex_bones[t.c]) key += triangle_count;
  if (key == 0) { rigid++; key = bone; }   // rigid triangles are grouped by their bone
  else { remap[t.a] = 1; remap[t.b] = 1; remap[t.c] = 1; }
  key *= material_count;
  if (materials) key += materials[i];
  keys[i] = key;
 }
 SortTriangles_00417a90(keys, 0, triangle_count - 1);
 operator delete(keys);
 transparent_count = transparent;
 first_triangle = rigid;
 message.Format("%d solid, %d transparent of %d polygons found.", rigid, transparent, triangle_count);
 PickingTrace(message);
 int soft = 0;
 Vec3M *new_normals = 0;
 int last = vertex_count - 1;
 Vec3M *new_vertices = new Vec3M[vertex_count];
 if (normals) new_normals = new Vec3M[vertex_count];
 int *new_bones = (int *)operator new(vertex_count * 4);
 for (i = 0; i < vertex_count; i++) {
  if (remap[i] == 0) remap[i] = last--;
  else remap[i] = soft++;
  new_vertices[remap[i]] = vertices[i];
  new_bones[remap[i]] = vertex_bones[i];
  if (normals) new_normals[remap[i]] = normals[i];
 }
 skin_count = soft;
 message.Format("%d software vertices of %d vertices", soft, vertex_count);
 PickingTrace(message);
 operator delete(vertices);
 operator delete(vertex_bones);
 if (new_normals) { operator delete(normals); normals = new_normals; }
 vertex_bones = new_bones;
 vertices = new_vertices;
 for (i = 0; i < triangle_count; i++) {
  triangles[i].a = remap[triangles[i].a];
  triangles[i].b = remap[triangles[i].b];
  triangles[i].c = remap[triangles[i].c];
 }
 operator delete(remap);
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

struct AnimationCache { // original global 0x5024a0
 int step, count;
 MeshRecord_00417940 *data;
 int Find_00415b40(ObservedString name);
 int FindFree_00415c60();
 bool Reserve_00415cc0(int size);
};
extern AnimationCache g_005024a0;
extern ResourceCache g_005ee6dc;
struct AnimRecordClear { ~AnimRecordClear(); };      // record destructor 0x417980 (Clear)
extern "C" char *__cdecl strcpy(char *, const char *);
#pragma intrinsic(strcpy)

static inline MeshRecord_00417940 *MeshAt(int index) {
 return (index >= 0 && index < g_005024a0.count) ? &g_005024a0.data[index] : 0;
}

// Claim a cache slot for a new mesh part.
static inline int NewMeshSlot(ObservedString &part) {
 int index = g_005024a0.FindFree_00415c60();
 if (index == -1) {
  int last = g_005024a0.count;
  g_005024a0.Reserve_00415cc0(g_005024a0.count + g_005024a0.step);
  index = last;
 }
 MeshRecord_00417940 &r = g_005024a0.data[index];
 r.used = index;
 r.age = -1;
 r.refs = 1;
 ObservedString copy(part);
 strcpy(r.name, copy.GetBuffer(0x20));
 copy.ReleaseBuffer(-1);
 return index;
}

static inline void ReadMaterials(VfsFile &in, ObservedString &dir, int count, int *ids) {
 char buffer[32];
 ObservedString file;
 for (int i = 0; i < count; i++) {
  in.Read_004209b0(buffer, 32);
  file.Format("%s", buffer);
  file = dir + file + ".MAT";
  ids[i] = g_005ee6dc.Load_00417390(file, 0xff, 0);
 }
}

// .meat mesh file: one or more mesh parts ("name", "name1", ...) put into the
// mesh cache; returns their slots. Versions 1/2/10 and 0x14/0x15 (normals).
void __cdecl LoadMeat_00418cd0(ObservedString file, int *count, int **indices) {
 ObservedString part, base;
 char magic[32];
 char name[32];
 *count = 0;
 VfsFile in;
 if (!in.Locate_00420420(file)) return;
 in.Open_004206b0(file);
 file.MakeLower();
 int slash = file.ReverseFind('\\') + 1;
 base = file.Mid(slash);
 file = file.Mid(0, slash);
 base = base.Left(base.ReverseFind('.'));
 strcpy(name, base.GetBuffer(0));
 in.Read_004209b0(magic, 4);
 int version;
 in.Read_004209b0(&version, 4);
 int parts = 0, material_count = 0, mesh_count, m, i;
 int *material_ids = 0;
 MeshRecord_00417940 *mesh = 0;
 unsigned char maps;
 if (version == 0x14 || version == 0x15) {
  in.Read_004209b0(&parts, 4);
  in.Read_004209b0(&material_count, 4);
  if (material_count > 0) {
   material_ids = (int *)operator new(material_count * 4);
   ReadMaterials(in, file, material_count, material_ids);
  }
  in.Read_004209b0(&mesh_count, 4);
  *count = mesh_count;
  *indices = (int *)operator new(mesh_count * 4);
  for (m = 0; m < mesh_count; m++) {
   if (m < 1) part = name;
   else part.Format("%s%i", name, m);
   int index;
   if (g_005024a0.Find_00415b40(part) != -1) { index = -1; mesh = 0; }   // sic: already loaded parts are not reused here
   else { index = NewMeshSlot(part); mesh = MeshAt(index); }
   mesh->part_count = parts;
   (*indices)[m] = index;
   in.Read_004209b0(&mesh->vertex_count, 4);
   mesh->vertices = new Vec3M[mesh->vertex_count];
   mesh->vertex_bones = (int *)operator new(mesh->vertex_count * 4);
   for (i = 0; i < mesh->vertex_count; i++) mesh->vertex_bones[i] = 0;
   for (i = 0; i < mesh->vertex_count; i++) {
    in.Read_004209b0(&mesh->vertices[i].x, 4);
    in.Read_004209b0(&mesh->vertices[i].y, 4);
    in.Read_004209b0(&mesh->vertices[i].z, 4);
   }
   if (version == 0x15) {
    mesh->has_normals = 1;
    mesh->normals = new Vec3M[mesh->vertex_count];
    for (i = 0; i < mesh->vertex_count; i++) {
     in.Read_004209b0(&mesh->normals[i].x, 4);
     in.Read_004209b0(&mesh->normals[i].y, 4);
     in.Read_004209b0(&mesh->normals[i].z, 4);
    }
   }
   in.Read_004209b0(&mesh->triangle_count, 4);
   mesh->triangles = (TriM *)operator new(mesh->triangle_count * sizeof(TriM));
   for (i = 0; i < mesh->triangle_count; i++) {
    in.Read_004209b0(&mesh->triangles[i].a, 4);
    in.Read_004209b0(&mesh->triangles[i].b, 4);
    in.Read_004209b0(&mesh->triangles[i].c, 4);
   }
   in.Read_004209b0(&maps, 1);
   if (maps) {
    mesh->materials = (int *)operator new(mesh->triangle_count * 4);
    for (i = 0; i < mesh->triangle_count; i++) in.Read_004209b0(&mesh->materials[i], 4);
    if (maps & 1) {
     mesh->flags |= 1;
     mesh->uv = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv[i].uv[k], 4);
    }
    if (maps & 2) {
     mesh->flags |= 2;
     mesh->uv2 = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv2[i].uv[k], 4);
    }
    if (maps & 4) {
     mesh->flags |= 4;
     mesh->uv3 = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv3[i].uv[k], 4);
    }
    if (maps & 8) mesh->flags |= 8;
   }
   if (parts) in.Read_004209b0(mesh->vertex_bones, mesh->vertex_count * 4);
   mesh->part_count = parts ? parts : 1;
   for (i = 0; i < mesh->triangle_count; i++) mesh->materials[i] = material_ids[mesh->materials[i]];
   mesh->Partition_00417c90();
   mesh->ComputeNormals_00418060();
   mesh->BuildLists_004182d0();
  }
 } else if (version == 1 || version == 2 || version == 10) {
  if (version == 2) in.Read_004209b0(&parts, 4);
  else parts = 1;
  in.Read_004209b0(&mesh_count, 4);
  *count = mesh_count;
  *indices = (int *)operator new(mesh_count * 4);
  for (m = 0; m < mesh_count; m++) {
   if (m < 1) part = base;
   else part.Format("%s%i", name, m);
   int index;
   if (g_005024a0.Find_00415b40(part) != -1) {
    // Reload an already cached part in place.
    index = g_005024a0.Find_00415b40(part);
    MeshRecord_00417940 *old = MeshAt(index);
    ((AnimRecordClear *)old)->~AnimRecordClear();
   } else index = NewMeshSlot(part);
   mesh = MeshAt(index);
   mesh->part_count = parts;
   (*indices)[m] = index;
   if (version == 2) { int skip; in.Read_004209b0(&skip, 4); }
   in.Read_004209b0(&mesh->vertex_count, 4);
   mesh->vertices = new Vec3M[mesh->vertex_count];
   mesh->vertex_bones = (int *)operator new(mesh->vertex_count * 4);
   mesh->part_count = parts ? parts : 1;
   for (i = 0; i < mesh->vertex_count; i++) mesh->vertex_bones[i] = 0;
   for (i = 0; i < mesh->vertex_count; i++) {
    in.Read_004209b0(&mesh->vertices[i].x, 4);
    in.Read_004209b0(&mesh->vertices[i].y, 4);
    in.Read_004209b0(&mesh->vertices[i].z, 4);
   }
   in.Read_004209b0(&mesh->triangle_count, 4);
   mesh->triangles = (TriM *)operator new(mesh->triangle_count * sizeof(TriM));
   for (i = 0; i < mesh->triangle_count; i++) {
    in.Read_004209b0(&mesh->triangles[i].a, 4);
    in.Read_004209b0(&mesh->triangles[i].b, 4);
    in.Read_004209b0(&mesh->triangles[i].c, 4);
   }
   maps = 1;
   if (version < 10 || (in.Read_004209b0(&maps, 1), maps != 0)) {
    mesh->materials = (int *)operator new(mesh->triangle_count * 4);
    for (i = 0; i < mesh->triangle_count; i++) in.Read_004209b0(&mesh->materials[i], 4);
   }
   if (version < 10) in.Read_004209b0(&maps, 1);
   if (maps) {
    if (maps & 1) {
     mesh->flags |= 1;
     mesh->uv = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv[i].uv[k], 4);
    }
    if (maps & 2) {
     mesh->flags |= 2;
     mesh->uv2 = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv2[i].uv[k], 4);
    }
    if (maps & 4) {
     mesh->flags |= 4;
     mesh->uv3 = (UvM *)operator new(mesh->triangle_count * sizeof(UvM));
     for (i = 0; i < mesh->triangle_count; i++)
      for (int k = 0; k < 6; k++) in.Read_004209b0(&mesh->uv3[i].uv[k], 4);
    }
    mesh->has_normals = 1;
    if (maps & 8) {
     mesh->normals = new Vec3M[mesh->vertex_count];
     for (i = 0; i < mesh->vertex_count; i++) {
      in.Read_004209b0(&mesh->normals[i].x, 4);
      in.Read_004209b0(&mesh->normals[i].y, 4);
      in.Read_004209b0(&mesh->normals[i].z, 4);
     }
    }
   }
   if ((version == 1 || version == 10) && (in.Read_004209b0(&material_count, 4), material_count > 0)) {
    material_ids = (int *)operator new(material_count * 4);
    ReadMaterials(in, file, material_count, material_ids);
    for (i = 0; i < mesh->triangle_count; i++) mesh->materials[i] = material_ids[mesh->materials[i]];
   }
   mesh->Partition_00417c90();
   mesh->ComputeNormals_00418060();
   mesh->BuildLists_004182d0();
  }
 }
 // original bug: an unknown version falls through here with mesh == 0 (writes to address 0x70)
 mesh->resources = material_ids;
 mesh->resource_count = material_count;
 in.Close_00420a60();
}
