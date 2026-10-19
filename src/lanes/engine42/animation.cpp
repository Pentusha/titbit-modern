#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Skeletal animation component (vtable 0x4cd670): EZA keyframe file loader and
// per-frame bone interpolation with quaternion slerp (0x4269f0-0x427300).
#include "lane_xml.h"
#include "e42_scene.h"
#include <math.h>
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
void __cdecl Log_00403650(const char *text);
extern Scene g_005011c0;

struct DomNode42 {
 virtual void s0();
 virtual void Release(int);
 virtual void s2(); virtual void s3(); virtual void s4();
 virtual const std::string *Name();
 virtual const std::string *Value();                 // +0x18
 virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
 virtual DomNode42 *FirstChild();
 virtual void s12(); virtual void s13();
 virtual DomNode42 *NextSibling();
 virtual struct DomAttributes42 *Attributes();       // +0x3c
 int refs;
};
struct DomAttributes42 {
 virtual void s0();
 virtual void Release(int);
 virtual DomNode42 *Named(const std::string &name);  // +0x08
};

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
 unsigned Read_004209b0(char *buffer, unsigned count);
 void Close_00420a60();
};

// Bone record of a game object (0xd8 bytes, see Bone_0040c6d0).
struct AnimBone {
 unsigned reserved0;
 float x, y, z;                     // 0x04
 float qx, qy, qz, qw;              // 0x10
 unsigned char rest[0xd8 - 0x20];
};
struct AnimOwner {
 unsigned char reserved0[0x10];
 struct ComponentE42 *components;   // 0x10
 unsigned char reserved14[0x34 - 0x14];
 int boneCount;                     // 0x34
 AnimBone *bones;                   // 0x38
 unsigned reserved3c;
 int dirty;                         // 0x40
};

struct ComponentE42 {
 virtual ObservedString Slot0();
 virtual void Slot1(int);
 virtual void Update(int time);                      // +0x08
 virtual ~ComponentE42();                            // +0x0c
 AnimOwner *owner;
 ComponentE42 *next;
 ComponentE42();                                     // 0x40b0f0
 void Unlink_0040b2a0();
};

struct Quat42 {
 float x, y, z, w;
 Quat42() {}
 Quat42(const Quat42 &q);                                                  // 0x4270e0
 void Set_00427100(const float &ax, const float &ay, const float &az, const float &aw);
 void Mul_0040df80(Quat42 *out, const Quat42 &q) const;
};
extern "C" double __stdcall FUN_0040df60(float);     // sin
extern "C" double __stdcall FUN_0040df70(float);     // cos

struct AnimClip_00406760 {
 int a, b, c;
 char name[0x20];
 int bone;                          // 0x2c
 int channel30;
 int px, py, pz;                    // 0x34 position channels (-1 = none)
 int rx, ry, rz;                    // 0x40 rotation channels (Euler degrees)
 AnimClip_00406760();
 ~AnimClip_00406760();              // folded empty dtor 0x4051b0
};

class SkeletonAnimation : public ComponentE42 {  // 0x4269f0 (was Animation_004269f0)
public:
 float time;                        // 0x0c
 int frameCount;                    // 0x10
 float frameTime;                   // 0x14
 int clipCount;                     // 0x18
 int channelCount;                  // 0x1c
 float duration;                    // 0x20
 AnimClip_00406760 *clips;          // 0x24
 float *frames;                     // 0x28
 int loops;                         // 0x2c
 ObservedString file;               // 0x30
 SkeletonAnimation(AnimOwner *owner);
 virtual ~SkeletonAnimation();
 virtual void Update(int time);
 void Load_00427130(DomNode42 *node);
 bool LoadFile_00427300(ObservedString path);
 void SetFrameRate_00427700(float fps);
 float GetFrameRate_00427740();
};

SkeletonAnimation::SkeletonAnimation(AnimOwner *anOwner) {
 clips = 0;
 frames = 0;
 time = 0;
 owner = anOwner;
}

// NONMATCHING: faithful translation
SkeletonAnimation::~SkeletonAnimation() {
 delete[] clips;
 ::operator delete(frames);
 time = 0;
}

Quat42::Quat42(const Quat42 &q) {
 x = q.x;
 y = q.y;
 z = q.z;
 w = q.w;
}

void Quat42::Set_00427100(const float &ax, const float &ay, const float &az, const float &aw) {
 x = ax;
 y = ay;
 z = az;
 w = aw;
}

static inline Quat42 EulerToQuat(float rx, float ry, float rz) {
 float h = rx * 0.5f * 0.017453292f;
 Quat42 qx; qx.x = (float)FUN_0040df60(h); qx.y = 0; qx.z = 0; qx.w = (float)FUN_0040df70(h);
 h = ry * 0.5f * 0.017453292f;
 Quat42 qy; qy.x = 0; qy.y = (float)FUN_0040df60(h); qy.z = 0; qy.w = (float)FUN_0040df70(h);
 h = rz * 0.5f * 0.017453292f;
 Quat42 qz; qz.x = 0; qz.y = 0; qz.z = (float)FUN_0040df60(h); qz.w = (float)FUN_0040df70(h);
 Quat42 zy, r;
 qz.Mul_0040df80(&zy, qy);
 zy.Mul_0040df80(&r, qx);
 Quat42 q;
 q.Set_00427100(r.x, r.y, r.z, r.w);
 return q;
}

static inline Quat42 Slerp(const Quat42 &from, const Quat42 &to, float t) {
 Quat42 to1;
 double cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
 if (cosom >= 0.0) { to1.x = to.x; to1.y = to.y; to1.z = to.z; to1.w = to.w; }
 else { cosom = -cosom; to1.x = -to.x; to1.y = -to.y; to1.z = -to.z; to1.w = -to.w; }
 double scale0, scale1;
 if (1.0 - cosom > 0.001) {
  double omega = acos(cosom);
  double sinom = sin(omega);
  scale0 = sin((1.0 - t) * omega) / sinom;
  scale1 = sin(t * omega) / sinom;
 } else {
  scale0 = 1.0 - t;
  scale1 = t;
 }
 Quat42 r;
 r.x = (float)(scale0 * from.x + scale1 * to1.x);
 r.y = (float)(scale0 * from.y + scale1 * to1.y);
 r.z = (float)(scale0 * from.z + scale1 * to1.z);
 r.w = (float)(scale0 * from.w + scale1 * to1.w);
 return r;
}

// Advances the animation clock and poses the owner's bones.
// NONMATCHING: faithful translation
void SkeletonAnimation::Update(int now) {
 if (frameCount == 0) return;
 time = (float)(unsigned)g_005026c0 * 0.001f + time;
 while (time >= duration) {
  time = time - duration;
  if (loops > 0) loops--;
  if (loops == 0) {
   ObservedPickingMessage message;
   memset(&message, 0, sizeof(message));
   message.selected = owner;
   message.event = 0xc;
   ((void **)message.reserved28)[0] = this;
   g_005011c0.Send_00424780(&message);
   Unlink_0040b2a0();
   delete this;
   return;
  }
 }
 float position = time / frameTime;
 int frame = (int)floor(position);
 float t = position - (float)frame;
 float u = 1.0f - t;
 float *current = frames + channelCount * frame;
 float *following = current + channelCount;
 for (int i = 0; i < clipCount; i++) {
  AnimClip_00406760 *clip = &clips[i];
  if (clip->bone >= owner->boneCount) continue;
  AnimBone *bone = &owner->bones[clip->bone];
  if (clip->px != -1) bone->x = t * following[clip->px] + u * current[clip->px];
  if (clip->py != -1) bone->y = t * following[clip->py] + u * current[clip->py];
  if (clip->pz != -1) bone->z = t * following[clip->pz] + u * current[clip->pz];
  float ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0;
  if (clip->rx != -1) { ax = current[clip->rx]; bx = following[clip->rx]; }
  if (clip->ry != -1) { ay = current[clip->ry]; by = following[clip->ry]; }
  if (clip->rz != -1) { az = current[clip->rz]; bz = following[clip->rz]; }
  Quat42 from = EulerToQuat(ax, ay, az);
  Quat42 to = EulerToQuat(bx, by, bz);
  Quat42 r(Slerp(to, from, u));
  bone->qx = r.x; bone->qy = r.y; bone->qz = r.z; bone->qw = r.w;
 }
 owner->dirty = 1;
 if (next) next->Update(now);
}

// <animation animation_name="file.eza"/>
// NONMATCHING: faithful translation
void SkeletonAnimation::Load_00427130(DomNode42 *node) {
 DomAttributes42 *attributes = node->Attributes();
 if (!attributes) return;
 DomNode42 *attribute = attributes->Named(std::string("animation_name"));
 if (attribute) {
  file = attribute->Value()->c_str();
  LoadFile_00427300(file);
 }
 loops = -1;
}

// NONMATCHING: faithful translation
bool SkeletonAnimation::LoadFile_00427300(ObservedString path) {
 ObservedString magic;
 VfsFile in;
 if (!in.Locate_00420420(path)) {
  Log_00403650("failed to open animation");
  return false;
 }
 in.Open_004206b0(path);
 magic = "EZA!_1.0";
 char header[12];
 in.Read_004209b0(header, 9);
 if (lstrcmpA(magic, header) != 0) return false;
 in.Read_004209b0((char *)&clipCount, 4);
 delete[] clips;
 clips = new AnimClip_00406760[clipCount];
 for (int i = 0; i < clipCount; i++) {
  in.Read_004209b0((char *)&clips[i].a, 4);
  in.Read_004209b0((char *)&clips[i].b, 4);
  in.Read_004209b0((char *)&clips[i].c, 4);
  in.Read_004209b0(clips[i].name, 0x20);
  in.Read_004209b0((char *)&clips[i].bone, 4);
  in.Read_004209b0((char *)&clips[i].channel30, 4);
  in.Read_004209b0((char *)&clips[i].px, 4);
  in.Read_004209b0((char *)&clips[i].py, 4);
  in.Read_004209b0((char *)&clips[i].pz, 4);
  in.Read_004209b0((char *)&clips[i].rx, 4);
  in.Read_004209b0((char *)&clips[i].ry, 4);
  in.Read_004209b0((char *)&clips[i].rz, 4);
 }
 in.Read_004209b0((char *)&channelCount, 4);
 in.Read_004209b0((char *)&frameCount, 4);
 in.Read_004209b0((char *)&frameTime, 4);
 duration = (float)(frameCount - 1) * frameTime;
 ::operator delete(frames);
 frames = (float *)::operator new(frameCount * channelCount * 4);
 in.Read_004209b0((char *)frames, frameCount * channelCount * 4);
 in.Close_00420a60();
 return true;
}

// Script SKEL_ANI.SetFrameRate: keeps the current frame position.
void SkeletonAnimation::SetFrameRate_00427700(float fps) {
 time = time / frameTime;
 frameTime = 1.0f / fps;
 time = frameTime * time;
 duration = (float)(frameCount - 1) * frameTime;
}

// Script SKEL_ANI.GetFrameRate
float SkeletonAnimation::GetFrameRate_00427740() {
 return 1.0f / frameTime;
}

// Native 0x406760, shared empty record constructor.
AnimClip_00406760::AnimClip_00406760(){}
