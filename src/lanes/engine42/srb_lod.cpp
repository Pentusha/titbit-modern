#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Skinned/rigid-body LOD component "srb_lod" (vtable 0x4cd6a4, 0x428320-0x428a20).
#include "../../native/engine_string.h"
extern "C" {
 __declspec(dllimport) void __stdcall glPushMatrix();
 __declspec(dllimport) void __stdcall glPopMatrix();
 __declspec(dllimport) void __stdcall glLoadMatrixf(const float *);
 __declspec(dllimport) void __stdcall glCallList(unsigned);
}
#include <math.h>
void __cdecl FormatArchiveString(ObservedString *, const char *, ...);

struct MeatMesh42 {                 // 0x78-byte mesh record
 char name[0x30];
 float *vertices;                   // 0x30
 unsigned char reserved34[0x60 - 0x34];
 int vertexCount;                   // 0x60
 bool hasNormals;                   // 0x64
 float *normals;                    // 0x68
 unsigned *lists;                   // 0x6c
 unsigned char reserved70[8];
};
extern int g_005024a4;              // mesh count
extern MeatMesh42 *g_005024a8;      // mesh table

struct LodOwner42 {
 unsigned char reserved0[0x34];
 int partCount;                     // 0x34
 unsigned char reserved38[4];
 float distance;                    // 0x3c distance to the camera
};
struct EngineChild42 {
 virtual void s0(); virtual int GetType(); virtual ObservedString Export(); virtual void Update(int time); virtual void Render(int time);
};

struct ComponentChild42 {virtual ObservedString Export();virtual void Update(int);virtual void Render(int);virtual ~ComponentChild42();};

struct AppendString42 : ObservedString {
 void Append_004075a0(int length, const char *text);
 void Add(const char *text) { Append_004075a0(lstrlenA(text), text); }
 void Add(const ObservedString &text) { Append_004075a0(text.GetLength(), text); }
};

struct LodMatrix42 {
 float m[16];
 LodMatrix42() {
  m[0] = m[5] = m[10] = m[15] = 1;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
 }
};

class Lod {  // 0x414030 (was Lod_00414030)
public:
 LodOwner42 *owner;                 // 0x04
 int reserved8;
 EngineChild42 *child;              // 0x0c
 ComponentChild42 *extra;              // 0x10
 unsigned char body[0xd4 - 0x14];
 float nearDistance, farDistance;   // 0xd4
 int mesh;                          // 0xdc
 float *vertices;                   // 0xe0
 float *normals;                    // 0xe4
 Lod();
 virtual ~Lod();
 virtual int GetType();
 virtual ObservedString ToXml();
 virtual void Update(int time);
 virtual void Render(int time);
 virtual void Draw();
 void Prepare_004147d0();
};

class SrbLod : public Lod {  // 0x428940 (was SrbLod_00428940)
public:
 LodMatrix42 matrix;                // 0xe8
 SrbLod(LodOwner42 *anOwner);
 virtual ~SrbLod();
 virtual int GetType(){return 3;}
 virtual ObservedString ToXml();
 virtual void Update(int time);
 virtual void Render(int time);
 virtual void Draw();
 void Transform_004284f0();
 MeatMesh42 *Mesh() { return (mesh >= 0 && mesh < g_005024a4) ? &g_005024a8[mesh] : 0; }
};

SrbLod::SrbLod(LodOwner42 *anOwner) {
 owner = anOwner;
 vertices = 0;
 normals = 0;
 reserved8 = 0;
}

SrbLod::~SrbLod() {
 reserved8 = 0;
}

// NONMATCHING: faithful translation
void SrbLod::Draw() {
 MeatMesh42 *m = Mesh();
 for (int i = 0; i < owner->partCount; i++) {
  glPushMatrix();
  glLoadMatrixf(matrix.m);
  glCallList(m->lists[i]);
  glPopMatrix();
 }
}

// Transforms the mesh vertices (and normals, without translation) by the LOD matrix.
// NONMATCHING: faithful translation
void SrbLod::Transform_004284f0() {
 LodMatrix42 rotation;
 MeatMesh42 *m = Mesh();
 if (m->hasNormals) {
  rotation = matrix;
  rotation.m[12] = 0;
  rotation.m[13] = 0;
  rotation.m[14] = 0;
 }
 const float *M = matrix.m;
 for (int i = 0; i < m->vertexCount; i++) {
  const float *v = &m->vertices[i * 3];
  float *o = &vertices[i * 3];
  o[0] = v[0] * M[0] + v[1] * M[4] + M[8] * v[2] + M[12];
  o[2] = v[1] * M[6] + M[10] * v[2] + v[0] * M[2] + M[14];
  o[1] = M[1] * v[0] + M[9] * v[2] + v[1] * M[5] + M[13];
  if (m->hasNormals) {
   const float *n = &m->normals[i * 3];
   const float *R = rotation.m;
   float *p = &normals[i * 3];
   p[0] = R[8] * n[2] + R[4] * n[1] + R[0] * n[0] + R[12];   // original summation order
   p[1] = R[9] * n[2] + R[5] * n[1] + R[1] * n[0] + R[13];
   p[2] = R[10] * n[2] + R[6] * n[1] + R[2] * n[0] + R[14];
   p = &normals[i * 3];
   float length = (float)sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
   if (length != 0.0f) {
    length = 1.0f / length;
    p[0] = length * p[0];
    p[1] = length * p[1];
    p[2] = length * p[2];
   }
  }
 }
}

// NONMATCHING: faithful translation
ObservedString SrbLod::ToXml() {
 AppendString42 xml;
 ObservedString number;
 xml.Add("<srb_lod ");
 xml.Add("fardistance=\"");
 FormatArchiveString(&number, "%.4f", (double)farDistance);
 xml.Add(number);
 xml.Add("\" ");
 xml.Add("neardistance=\"");
 FormatArchiveString(&number, "%.4f", (double)nearDistance);
 xml.Add(number);
 xml.Add("\" ");
 xml.Add("meatname=\"");
 FormatArchiveString(&number, "%s.meat", Mesh());
 xml.Add(number);
 xml.Add("\" ");
 xml.Add("/>");
 xml.Add("\n\r");                                   // original: LF CR
 return xml;
}

// NONMATCHING: faithful translation
void SrbLod::Update(int time) {
 if (extra) extra->Update(time);
 if (owner->distance < farDistance && nearDistance < owner->distance) {
  Prepare_004147d0();
  Draw();
 }
 if (child) child->Update(time);
}

// Native 0x428390: retain translation, replace each basis column by its scale.
void SrbLod::Render(int pass){
 if(extra)extra->Render(pass);
 if(owner->distance<farDistance && nearDistance<owner->distance){
  struct Bone {char head[0x54];float world[16];char tail[0xd8-0x94];};
  Bone *bones=*(Bone**)((char*)owner+0x38);
  const float *m=bones[reserved8].world;
  for(int i=0;i<16;++i)matrix.m[i]=m[i];
  for(int c=0;c<3;++c){int at=c*4;float x=matrix.m[at],y=matrix.m[at+1],z=matrix.m[at+2];
   matrix.m[at]=matrix.m[at+1]=matrix.m[at+2]=0;
   matrix.m[at+c]=(float)sqrt(x*x+y*y+z*z);
  }
  Transform_004284f0();
 }
 if(child)child->Render(pass);
}
