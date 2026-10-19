// Camera part of the frame renderer (matrix at +0xebfc4, zoom +0xec004,
// fov +0xec008): view loading, look-at, and the CAMERA script calls.
#include <math.h>
extern "C" __declspec(dllimport) void __stdcall glLoadMatrixf(const float *);
#pragma intrinsic(sin, cos, sqrt, atan2)
extern "C" double __stdcall FUN_0040df60(float);   // sin
extern "C" double __stdcall FUN_0040df70(float);   // cos

struct Vec3C {
 float x, y, z;
 Vec3C() {}
 Vec3C(float a, float b, float c);                     // 0x406ee0
 Vec3C(const Vec3C &o);                                // 0x406f00
 Vec3C operator-(const Vec3C &o) const;                // 0x406f20
};

#define kDegToRad 0.017453292f

struct Mat4C {
 float m[16];
 Mat4C() { Identity(); }
 void Identity();                                      // 0x40cc50
 void SetIdentity() {
  m[0] = m[5] = m[10] = m[15] = 1.0f;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
 }
 Mat4C Multiply_0040cc90(const Mat4C &b) const;      // returned by value (hidden pointer)
 void RotateX_0041fdd0(float degrees);
 void RotateY_0041fe00(float degrees);
 void RotateZ_0041fe30(float degrees);
 void RotateXInline(float a) { a *= kDegToRad; float c = (float)cos(a), s = (float)sin(a); m[5] = c; m[6] = s; m[9] = -s; m[10] = c; }
 void RotateYInline(float a) { a *= kDegToRad; float c = (float)cos(a), s = (float)sin(a); m[0] = c; m[2] = -s; m[8] = s; m[10] = c; }
 void RotateZInline(float a) { a *= kDegToRad; float c = (float)cos(a), s = (float)sin(a); m[0] = c; m[1] = s; m[4] = -s; m[5] = c; }
 void LookAt_0041fe60(Vec3C eye, Vec3C target, Vec3C up);
 void Translate_00420200(const Vec3C &v);
 void Translate_0041fba0(float x, float y, float z) {
  m[12] = x * m[0] + z * m[8] + y * m[4] + m[12];
  m[13] = z * m[9] + y * m[5] + x * m[1] + m[13];
  m[14] = z * m[10] + y * m[6] + x * m[2] + m[14];
  m[15] = z * m[11] + y * m[7] + x * m[3] + m[15];
 }
};

struct BoneView { char head[0x94]; Mat4C world; };
struct ObjectView { char head[0x38]; BoneView *bone; };

class CameraRenderer_005026d0 {
public:
 char body[0xebfc4];
 Mat4C view;                          // +0xebfc4
 float zoom;                          // +0xec004
 float fov;                           // +0xec008
 void LoadView_0041f860();
 void SetFromMatrix_0041fc40(Mat4C m);
 void SetCamera_0041ffc0(ObjectView *object);
 void PlaceCamera_0041fff0(float fov, Vec3C position, Vec3C angles);
 void RotateAD_0041f980(float x, float y, float z, float px, float py, float pz, float degrees);
};

void CameraRenderer_005026d0::LoadView_0041f860() {
 Mat4C scaled;
 Mat4C scale;
 if (zoom != 1.0f) {
  scale.m[0] = zoom;
  scale.m[5] = zoom;
  scale.m[10] = zoom;
  scaled = view;
  scaled = scaled.Multiply_0040cc90(scale);
  scaled.m[12] *= zoom;
  scaled.m[13] *= zoom;
  glLoadMatrixf(scaled.m);
  return;
 }
 glLoadMatrixf(view.m);
}

// gluLookAt-style view matrix.
void Mat4C::LookAt_0041fe60(Vec3C eye, Vec3C target, Vec3C up) {
 float fx = eye.x - target.x, fy = eye.y - target.y, fz = eye.z - target.z;
 float length = (float)sqrt(fx * fx + fy * fy + fz * fz);
 if (length != 0.0f) { length = 1.0f / length; fx = length * fx; fy = length * fy; fz = length * fz; }
 length = (float)sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
 if (length != 0.0f) { length = 1.0f / length; up.x = up.x * length; up.y = up.y * length; up.z = length * up.z; }
 float sx = up.y * fz - fy * up.z;
 m[1] = up.x;
 float sy = fx * up.z - fz * up.x;
 float sz = fy * up.x - fx * up.y;
 m[0] = sx;
 m[4] = sy;
 m[5] = up.y;
 m[9] = up.z;
 m[8] = sz;
 m[2] = fx;
 m[6] = fy;
 m[10] = fz;
 m[12] = -(eye.x * sx) - eye.y * sy - eye.z * sz;
 m[13] = -(eye.x * up.x) - eye.y * up.y - eye.z * up.z;
 m[14] = -(eye.x * fx) - eye.y * fy - eye.z * fz;
}

// Camera from an object's world matrix: Euler angles out of the rotation part,
// eye 10 units above the object's origin.
void CameraRenderer_005026d0::SetFromMatrix_0041fc40(Mat4C m) {
 Vec3C eye;
 eye.x = m.m[12];
 eye.y = m.m[13];
 eye.z = m.m[14] + 10.0f;
 Mat4C r;
 float a, b, c;
 if (!(m.m[2] < 1.0f)) { a = (float)atan2(m.m[4], m.m[5]); b = 1.5707964f; c = 0.0f; }   // original test: also taken for NaN
 else if (m.m[2] > -1.0f) { a = (float)atan2(m.m[6], m.m[10]); b = (float)-asin(m.m[2]); c = (float)atan2(m.m[1], m.m[0]); }
 else { a = (float)-atan2(m.m[4], m.m[5]); b = -1.5707964f; c = 0.0f; }
 r.RotateX_0041fdd0(a * -57.295776f);
 r.RotateY_0041fe00(b * -57.295776f);
 r.RotateZ_0041fe30(c * -57.295776f);
 Vec3C side;
 side.x = r.m[0];
 side.y = r.m[4];
 side.z = r.m[8];
 Vec3C forward(r.m[2], r.m[6], r.m[10]);
 view.LookAt_0041fe60(eye, eye - side, forward);
}

void CameraRenderer_005026d0::SetCamera_0041ffc0(ObjectView *object) {
 SetFromMatrix_0041fc40(object->bone->world);
}

void CameraRenderer_005026d0::PlaceCamera_0041fff0(float fov_, Vec3C position, Vec3C angles) {
 Vec3C a(angles);
 Vec3C p(position);
 Mat4C r;
 view.SetIdentity();
 fov = fov_;
 r.SetIdentity();
 r.RotateXInline(a.x);
 view = r.Multiply_0040cc90(view);
 r.SetIdentity();
 r.RotateYInline(a.y);
 view = r.Multiply_0040cc90(view);
 r.Identity();
 r.RotateZInline(a.z);
 view = r.Multiply_0040cc90(view);
 view.Translate_00420200(p);
}

// Rotate the view by `degrees` around axis (x, y, z) through point (px, py, pz).
void CameraRenderer_005026d0::RotateAD_0041f980(float x, float y, float z, float px, float py, float pz, float degrees) {
 view.Translate_0041fba0(px, py, pz);
 Mat4C r;
 degrees *= kDegToRad;
 float c = (float)FUN_0040df70(degrees);
 float s = (float)FUN_0040df60(degrees);
 float t = 1.0f - c;
 r.m[0] = x * t * x + c;
 r.m[1] = z * s + y * t * x;
 r.m[4] = y * t * x - z * s;
 r.m[2] = z * t * x - y * s;
 r.m[8] = y * s + z * t * x;
 r.m[5] = y * y * t + c;
 r.m[6] = s * x + z * y * t;
 r.m[9] = z * y * t - s * x;
 r.m[10] = z * z * t + c;
 view = view.Multiply_0040cc90(r);
 view.Translate_0041fba0(-px, -py, -pz);
}
