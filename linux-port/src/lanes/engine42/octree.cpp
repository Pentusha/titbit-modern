#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Scene octree (DigiBen-style): build, sort, assign (0x423a80, 0x426030-0x426760)
// and the scene resource / string table readers (0x422a90, 0x424230).
#include "lane_xml.h"
#include "e42_scene.h"
#include <math.h>

struct DomNode42 {
 virtual void s0();
 virtual void Release(int);
 virtual void s2(); virtual void s3(); virtual void s4();
 virtual const std::string *Name();                  // +0x14
 virtual const std::string *Value();                 // +0x18
 virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
 virtual DomNode42 *FirstChild();                    // +0x2c
 virtual void s12(); virtual void s13();
 virtual DomNode42 *NextSibling();                   // +0x38
 virtual struct DomAttributes42 *Attributes();       // +0x3c
 int refs;
};
struct DomAttributes42 {
 virtual void s0();
 virtual void Release(int);                          // +0x04
 virtual DomNode42 *Named(const std::string &name);  // +0x08
 virtual void s3(); virtual void s4();
 virtual DomNode42 *Item(unsigned index);            // +0x14
 virtual unsigned Length();                          // +0x18
 int refs;
};

int SetArchiveRoot(const char *name);                // 0x420e60
int MountArchive(const char *name);                  // 0x420c70
int AddArchiveFolder_00420ab0(const char *name);
ObservedString __cdecl Unescape_0042a8b0(ObservedString text);
struct TextTable42 { void Add_0042a530(ScenePairElement *pair); };
struct ObjectList42 : ScenePtrArray<SceneObject *> { void SetSize_00411f60(int size, int grow); };

struct Vec3Ops : Vec3_00406f00 {
 Vec3Ops() {}
 Vec3Ops(float ax, float ay, float az);                        // 0x406ee0
 Vec3Ops Scaled_00416780(float s) const;
 Vec3_00406f00 Plus_00423d60(const Vec3Ops &o) const;
 Vec3Ops &Minus_00423d30(const Vec3Ops &o);
};

struct OctNode42 {
 int count;
 SceneObject **objects;
 bool split;
 OctNode42 *children[8];
 Vec3_00406f00 center;
 float width;
 OctNode42() : center(0, 0, 0) {
  split = false; count = 0; objects = 0;
  for (int i = 0; i < 8; ++i) children[i] = 0;
 }
 ~OctNode42() {                                      // 0x426640 (out of line for delete; inline in ~Spatial)
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

class Spatial {  // 0x426710 (was Spatial_00426710)
public:
 virtual void Bounds(SceneObject **item, Vec3_00406f00 *max, Vec3_00406f00 *min);
 int maxDepth, minObjects;
 ScenePtrArray<SceneObject *> storage;
 OctNode42 root;
 ~Spatial();
 void Build_00426030(OctNode42 *node, OctNode42 **owners, int depth);
 void Sort_00426530(OctNode42 **owners, int lo, int hi);
 void Assign_004265e0(OctNode42 *node, OctNode42 **owners);
};

// NONMATCHING: faithful translation
Spatial::~Spatial() {}                                // root's destructor runs inline

// Distributes the objects owned by `node` into its eight children, recursively.
// NONMATCHING: faithful translation
void Spatial::Build_00426030(OctNode42 *node, OctNode42 **owners, int depth) {
 int count = 0;
 int i;
 for (i = 0; i < storage.size; i++)
  if (owners[i] == node) count++;
 node->count = count;
 if (count < minObjects || depth >= maxDepth) return;
 float size = node->width * 0.5f;
 for (i = 0; i < storage.size; i++) {
  if (owners[i] != node) continue;
  Vec3_00406f00 max(0, 0, 0), min(0, 0, 0);
  Bounds(&storage.data[i], &max, &min);
  float cx = node->center.x, cy = node->center.y, cz = node->center.z;
  bool px = min.x >= cx && max.x <= cx + size, nx = max.x <= cx && min.x >= cx - size;
  bool py = min.y >= cy && max.y <= cy + size, ny = max.y <= cy && min.y > cy - size;
  bool pz = min.z > cz && max.z <= cz + size, nz = max.z <= cz && min.z >= cz - size;
  int octant;
  if (nx && ny && pz) octant = 0;
  else if (nx && py && pz) octant = 1;
  else if (px && py && pz) octant = 2;
  else if (px && ny && pz) octant = 3;
  else if (nx && ny && nz) octant = 4;
  else if (nx && py && nz) octant = 5;
  else if (px && py && nz) octant = 6;
  else if (px && ny && nz) octant = 7;
  else continue;
  float q = size * 0.5f;
  Vec3_00406f00 c(octant == 2 || octant == 3 || octant == 6 || octant == 7 ? cx + q : cx - q,
                  octant == 1 || octant == 2 || octant == 5 || octant == 6 ? cy + q : cy - q,
                  octant < 4 ? cz + q : cz - q);
  if (node->children[octant] == 0) {
   OctNode42 *child = new OctNode42;
   node->children[octant] = child;
   child->center.x = c.x; child->center.y = c.y; child->center.z = c.z;
   node->children[octant]->width = size;
   node->split = true;
  }
  count--;
  owners[i] = node->children[octant];
 }
 node->count = count;
 if (node->split == true)
  for (int k = 0; k < 8; k++)
   if (node->children[k]) Build_00426030(node->children[k], owners, depth + 1);
}

// Quicksort of the owner table, keeping storage in step.
// NONMATCHING: faithful translation
void Spatial::Sort_00426530(OctNode42 **owners, int lo, int hi) {
 int i;
 do {
  OctNode42 *pivot = owners[(lo + hi) / 2];
  int j = hi;
  i = lo;
  do {
   // the original compares the node pointers as signed ints
   while ((int)owners[i] < (int)pivot && i < hi) i++;
   while ((int)pivot < (int)owners[j] && lo < j) j--;
   if (j < i) break;
   OctNode42 *o = owners[i]; owners[i] = owners[j]; owners[j] = o;
   i++; j--;
   SceneObject *s = storage.data[i - 1]; storage.data[i - 1] = storage.data[j + 1]; storage.data[j + 1] = s;
  } while (i <= j);
  if (lo < j) Sort_00426530(owners, lo, j);
  lo = i;
 } while (i < hi);
}

// NONMATCHING: faithful translation
void Spatial::Assign_004265e0(OctNode42 *node, OctNode42 **owners) {
 for (int i = 0; i < storage.size; i++)
  if (owners[i] == node) { node->objects = &storage.data[i]; break; }
 if (node->split == true)
  for (int k = 0; k < 8; k++)
   if (node->children[k]) Assign_004265e0(node->children[k], owners);
}

// NONMATCHING: faithful translation
void Scene::BuildOctree_00423a80(bool update) {
 Spatial *tree = (Spatial *)&spatial;
 int count = statics.size;
 int i;
 for (i = 0; i < count; i++) {
  if (update) statics.data[i]->Update(g_005026bc);
  int index = tree->storage.size;
  SceneObject *object = statics.data[i];
  ((ObjectList42 *)&tree->storage)->SetSize_00411f60(index + 1, -1);
  tree->storage.data[index] = object;
 }
 OctNode42 **owners = (OctNode42 **)::operator new(tree->storage.size * sizeof(OctNode42 *));
 tree->maxDepth = 10;
 Vec3Ops lo, hi;
 lo.x = lo.y = lo.z = 1e6f;
 hi.x = hi.y = hi.z = -1e6f;
 for (i = 0; i < tree->storage.size; i++) {
  Vec3_00406f00 max(0, 0, 0), min(0, 0, 0);
  tree->Bounds(&tree->storage.data[i], &max, &min);
  if (min.x < lo.x) lo.x = min.x;
  if (min.y < lo.y) lo.y = min.y;
  if (min.z < lo.z) lo.z = min.z;
  if (hi.x < max.x) hi.x = max.x;
  if (hi.y < max.y) hi.y = max.y;
  if (hi.z < max.z) hi.z = max.z;
  owners[i] = &tree->root;
 }
 Vec3Ops extent(hi.x - lo.x, hi.y - lo.y, hi.z - lo.z);
 tree->root.center = lo.Plus_00423d60(extent.Scaled_00416780(0.5f));
 hi.Minus_00423d30(lo);
 float width = hi.x;
 if (width <= 0) width = 0;
 if (width < hi.y) width = (float)fabs(hi.y);
 if (width < hi.z) width = hi.z;
 tree->root.width = width;
 tree->root.count = tree->storage.size;
 tree->Build_00426030(&tree->root, owners, 0);
 tree->Sort_00426530(owners, 0, tree->storage.size - 1);
 tree->Assign_004265e0(&tree->root, owners);
 ::operator delete(owners);
}

// <resources><PATH path=".."/><PACKFILE path=".."/><DISKFILE path=".."/></resources>
// NONMATCHING: faithful translation
void Scene::ReadResources_00422a90(DomNode42 *node) {
 std::string path;
 for (; node; node = node->NextSibling()) {
  if (*node->Name() != "resources") continue;
  for (DomNode42 *child = node->FirstChild(); child; child = child->NextSibling()) {
   path = *child->Name();
   DomAttributes42 *attributes = child->Attributes();
   if (!attributes) continue;
   DomNode42 *attribute = attributes->Named(std::string("path"));
   if (attribute) {
    path = *attribute->Value();
    if (*child->Name() == "PATH") SetArchiveRoot(path.c_str());
    else if (*child->Name() == "PACKFILE") MountArchive(path.c_str());
    else if (*child->Name() == "DISKFILE") AddArchiveFolder_00420ab0(path.c_str());
   }
   if (--attributes->refs < 1) attributes->Release(1);
  }
 }
}

// <string_table key="text" .../>: localized strings into the pair table.
// NONMATCHING: faithful translation
void Scene::ReadAmbient_00424230(DomNode42 *node) {
 std::string unused;
 for (; node; node = node->NextSibling()) {
  if (*node->Name() != "string_table") continue;
  DomAttributes42 *attributes = node->Attributes();
  for (unsigned i = 0; i < attributes->Length(); i++) {
   DomNode42 *attribute = attributes->Item(i);
   ScenePairElement pair;
   pair.first = attribute->Name()->c_str();
   pair.second = attribute->Value()->c_str();
   pair.second = Unescape_0042a8b0(pair.second);
   ((TextTable42 *)&pairs)->Add_0042a530(&pair);
  }
  if (--attributes->refs < 1) attributes->Release(1);
 }
}

Vec3Ops &Vec3Ops::Minus_00423d30(const Vec3Ops &o) {
 x -= o.x;
 y -= o.y;
 z -= o.z;
 return *this;
}

Vec3_00406f00 Vec3Ops::Plus_00423d60(const Vec3Ops &o) const {
 return Vec3_00406f00(o.x + x, o.y + y, o.z + z);
}
