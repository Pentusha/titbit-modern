// Scene-object base methods (vtable 0x4cd5c0) and the LOD object (vtable 0x4cd5d4).
#include "e41_engine.h"
#include "lane_xml.h"

ObservedString EngineObject::Export() {
 ObservedString xml;
 return xml;
}

void EngineObject::Update(int time) {
 if (extra) extra->Update(time);
 if (next) next->Update(time);
}

void EngineObject::Render(int pass) {
 if (extra) extra->Render(pass);
 if (next) next->Render(pass);
}

void EngineObject::AppendNext_00413820(EngineObject *object) {
 EngineObject *p = next;
 if (p) {
  EngineObject *q;
  do { q = p; p = q->next; } while (p);
  q->next = object;
  return;
 }
 next = object;
}

// Split the parent's chain: objects owned (directly or not) by this one move to
// this object's chain, the rest stay with the parent.
void EngineObject::Detach_00413850() {
 EngineObject *keep = owner;
 EngineObject *mine = this;
 EngineObject *p = keep->next;
 owner = 0;
 for (; p; p = p->next) {
  if (p == this) continue;
  EngineObject *a = p->owner;
  while (a && a != this && a != owner) a = a->owner;
  if (a == this) { mine->next = p; mine = p; }
  else { keep->next = p; keep = p; }
 }
 mine->next = 0;
 keep->next = 0;
}

// LINK.Delete
void EngineObject::Delete_00413f50() {
 Detach_00413850();
 if (this) delete this;
}

// Same writer as 0x413220, compiled into this unit as well.
ObservedString __cdecl WriteVec3_00413cc0(const Vec3Z *v, ObservedString name) {
 ObservedString number;
 name = "<" + name;
 name += " ";
 name += "x=\"";
 number.Format("%.4f", (double)v->x);
 name += number;
 name += "\" ";
 name += "y=\"";
 number.Format("%.4f", (double)v->y);
 name += number;
 name += "\" ";
 name += "z=\"";
 number.Format("%.4f", (double)v->z);
 name += number;
 name += "\" ";
 name += "/>";
 name += "\n\r";
 return name;
}

struct Mat4I {
 float m[16];
 Mat4I() {
  m[0] = m[5] = m[10] = m[15] = 1.0f;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
 }
};

// Shared mesh ("meat") records: global array {count 0x5024a4, items 0x5024a8}.
struct MeatRecord { ObservedString name; char body[0x20]; int refs; char tail[0x50]; };
extern "C" int g_005024a4;
extern "C" MeatRecord *g_005024a8;
static inline MeatRecord *GetMeat(int index) {
 return (index >= 0 && index < g_005024a4) ? &g_005024a8[index] : 0;
}

struct OwnerView { char head[0x3c]; float distance; };

class Lod : public EngineObject { // original 0x414150
public:
 Mat4I m0, m1, m2;                    // +0x14 +0x54 +0x94
 float near_distance, far_distance;   // +0xd4 +0xd8
 int meat;                            // +0xdc
 void *buffer0, *buffer1;             // +0xe0 +0xe4
 Lod();
 Lod(EngineObject *owner);
 virtual ~Lod();
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 void Draw_00415190();
 virtual void Draw();
 void DrawMesh_004147d0();
};

Lod::Lod() { buffer0 = 0; buffer1 = 0; }
Lod::Lod(EngineObject *owner_) { owner = owner_; buffer0 = 0; buffer1 = 0; }

Lod::~Lod() {
 if (buffer0) operator delete(buffer0);
 if (buffer1) operator delete(buffer1);
 buffer0 = 0;
 buffer1 = 0;
 if (meat >= 0 && meat < g_005024a4) {
  int &refs = g_005024a8[meat].refs;
  if (refs) refs--;
 }
}

int Lod::GetType() { return 1; }

ObservedString Lod::Export() {
 ObservedString xml, number;
 xml += "<lod ";
 xml += "meatname=\"";
 number.Format("%s.meat", GetMeat(meat));
 xml += number;
 xml += "\" ";
 xml += "neardistance=\"";
 number.Format("%.4f", (double)near_distance);
 xml += number;
 xml += "\" ";
 xml += "fardistance=\"";
 number.Format("%.4f", (double)far_distance);
 xml += number;
 xml += "\" ";
 xml += "/>";
 xml += "\n\r";
 return xml;
}

void Lod::Render(int pass) {
 if (extra) extra->Render(pass);
 if (((OwnerView *)owner)->distance < far_distance && ((OwnerView *)owner)->distance > near_distance)
  Draw_00415190();
 if (next) next->Render(pass);
}

// Third copy of the x/y/z attribute reader (0x4058c0, 0x412fa0), in this unit.
Vec3V __cdecl ReadVec3_00413a20(XmlNode_v *node) {
 float z = 0, y = 0, x = 0;
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("x"));
  if (a) x = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("y"));
  if (a) y = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("z"));
  if (a) z = (float)atof(a->Value()->c_str());
  if (--element->refs <= 0) element->Destroy(1);
 }
 return Vec3V(x, y, z);
}

// Native 0x413a10, base object has no concrete type.
int EngineObject::GetType(){return -1;}

// Native virtual draw 0x415020: submit one display list per owner bone.
extern "C" {
 __declspec(dllimport) void __stdcall glPushMatrix();
 __declspec(dllimport) void __stdcall glPopMatrix();
 __declspec(dllimport) void __stdcall glLoadMatrixf(const float *);
 __declspec(dllimport) void __stdcall glCallList(unsigned);
}
void Lod::Draw(){
 struct Bone {char head[0x54];float matrix[16];char tail[0xd8-0x94];};
 struct Owner {char head[0x34];int count;Bone *bones;};
 Owner *p=(Owner*)owner;
 unsigned *lists=*(unsigned**)((char*)GetMeat(meat)+0x6c);
 for(int i=0;i<p->count;++i){glPushMatrix();glLoadMatrixf(p->bones[i].matrix);glCallList(lists[i]);glPopMatrix();}
}

// Native 0x428a20: shared LOD update/render-list submission.
void Lod::Update(int time){
 if(extra)extra->Update(time);
 if(((OwnerView*)owner)->distance<far_distance && ((OwnerView*)owner)->distance>near_distance){DrawMesh_004147d0();Draw();}
 if(next)next->Update(time);
}
