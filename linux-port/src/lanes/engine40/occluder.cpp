// "Stupid occluder" model (vtable 0x4cd454): triangles copied from a loaded
// mesh ("source_meat"). Constructor/destructor: lanes/claude/engine_objects.cpp.
#define E40_LOADSTRING_CALL   // the original unit calls CString::LoadString 0x406520
#include "e40_engine.h"
#include "lane_xml.h"
#include "e40_gl.h"
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);

struct CVec3 {
 float x, y, z;
 CVec3() { x = y = z = 0; }
 CVec3(const CVec3 &o) { x = o.x; y = o.y; z = o.z; }   // out-of-line copy: 0x406f00
};

/* The constructor is inline in this unit, so the unit emits the Triangle
 * vtable (0x4cd6bc) and with it the vector deleting destructor 0x406430. */
class Triangle {  // 0x428ab0 (was Triangle_00428ab0)
public:
 Vec3Z a, b, c, normal;
 float d;
#ifdef TITBIT_SOURCE_LINK
 Triangle();                         // canonical provider: engine42/collision.cpp
#else
 Triangle() {}
#endif
 virtual ~Triangle();
 void Set_00428af0(CVec3 a, CVec3 b, CVec3 c);
 float Collide_00429060(CVec3 position, CVec3 direction, float distance, float radius, CVec3 *slide, float *slideLength);
 bool RayHits_004295e0(CVec3 origin, CVec3 direction, float *t);
};

struct MeshRecord {                   // animation/mesh cache record, 0x78 bytes
 char name[0x30];
 CVec3 *vertices;                     // +0x30
 int pad34, pad38;
 int faceCount;                       // +0x3c
 int *faces;                          // +0x40 (3 vertex indices per face)
 char tail[0x78 - 0x44];
};

struct MeshCache_005024a0 {
 int step, count;
 MeshRecord *data;
 int Find(ObservedString name) {
  name.MakeLower();
  for (int i = 0; i < count; i++)
   if (lstrcmpA(name, data[i].name) == 0) return i;
  return -1;
 }
 MeshRecord *At(int index) { return (index >= 0 && index < count) ? &data[index] : 0; }
};
extern MeshCache_005024a0 g_005024a0;

// Model base (Node_00405160, vtable 0x4cd40c): collision queries after the
// five EngineObject slots.
class Node_00405160 : public EngineObject {
public:
 virtual int GetType();                                  // 0x4051c0
 virtual bool RayTest(CVec3 origin, CVec3 direction, float *distance);   // 0x4051e0
 virtual void Slot6();                                   // 0x4051b0 (empty)
 virtual float Collide(CVec3 position, CVec3 direction, float distance, float radius,
                       CVec3 *slide, float *slideLength); // 0x4051d0
};

class Model_00405e90 : public Node_00405160 {
public:
 virtual void Update(int time);
 virtual bool RayTest(CVec3 origin, CVec3 direction, float *distance);
 virtual float Collide(CVec3 position, CVec3 direction, float distance, float radius,
                       CVec3 *slide, float *slideLength);
 int meat;                            // 0x14
 Triangle *faces;            // 0x18
 void *buffer;                        // 0x1c
 int faceCount;                       // 0x20
 void Load_00405fa0(XmlNode_v *node);
 void BuildFaces_00406300(MeshRecord *mesh);
};

void Model_00405e90::Load_00405fa0(XmlNode_v *node) {
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("source_meat"));
  if (a) {
   ObservedString name = a->Value()->c_str();
   meat = g_005024a0.Find(name);
   if (meat != -1) BuildFaces_00406300(g_005024a0.At(meat));
  }
  if (--element->refs <= 0) element->Destroy(1);
 }
}

void Model_00405e90::BuildFaces_00406300(MeshRecord *mesh) {
 faceCount = mesh->faceCount;
 faces = new Triangle[faceCount];
 for (int i = 0; i < mesh->faceCount; i++) {
  int *f = &mesh->faces[i * 3];
  faces[i].Set_00428af0(mesh->vertices[f[0]], mesh->vertices[f[1]], mesh->vertices[f[2]]);
 }
}

void Model_00405e90::Update(int time) {
 if (next) next->Update(time);
}

// Nearest hit of the ray over all triangles (100000 when none).
bool Model_00405e90::RayTest(CVec3 origin, CVec3 direction, float *distance) {
 *distance = 100000.0f;
 bool hit = false;
 for (int i = 0; i < faceCount; i++) {
  float t;
  if (faces[i].RayHits_004295e0(origin, direction, &t) && t < *distance) {
   hit = true;
   *distance = t;
  }
 }
 return hit;
}

// Sphere sweep over all triangles: the shortest free distance and its slide.
float Model_00405e90::Collide(CVec3 position, CVec3 direction, float distance, float radius,
                              CVec3 *slide, float *slideLength) {
 float best = distance;
 CVec3 s;
 for (int i = 0; i < faceCount; i++) {
  float length;
  float d = faces[i].Collide_00429060(position, direction, distance, radius, &s, &length);
  if (d < best) {
   best = d;
   *slideLength = length;
   slide->x = s.x; slide->y = s.y; slide->z = s.z;
  }
 }
 return best;
}

int Node_00405160::GetType() { return 4; }
bool Node_00405160::RayTest(CVec3 origin, CVec3 direction, float *distance) { return false; }
float Node_00405160::Collide(CVec3 position, CVec3 direction, float distance, float radius,
                             CVec3 *slide, float *slideLength) { return distance; }

// Box collision model (vtable 0x4cd42c; box triangles: collision_box.cpp):
// draws the twelve triangles as lines in the space of the owner's first bone.
struct BoxBone { char head[0x54]; float matrix[16]; char tail[0xd8 - 0x94]; };
struct BoxOwner { char head[0x38]; BoxBone *bones; };

class BoxedModel_004057f0 : public Node_00405160 {
public:
 Triangle faces[12];                  // 0x14
 float sx, sy, sz;                    // 0x2b4
 virtual void Update(int time);
};

void BoxedModel_004057f0::Update(int time) {
 glPushMatrix();
 glMultMatrixf(((BoxOwner *)owner)->bones->matrix);
 glBegin(1);                          // GL_LINES
 for (int i = 0; i < 12; i++) {
  glVertex3fv(&faces[i].a.x);
  glVertex3fv(&faces[i].b.x);
  glVertex3fv(&faces[i].b.x);
  glVertex3fv(&faces[i].c.x);
  glVertex3fv(&faces[i].c.x);
  glVertex3fv(&faces[i].a.x);
 }
 glEnd();
 glPopMatrix();
 if (next) next->Update(time);
}
