// Frame renderer state (global at 0x5026d0): per-frame light list, GL light
// setup, and the camera matrix helpers.
#include <math.h>
extern "C" {
__declspec(dllimport) void __stdcall glEnable(int);
__declspec(dllimport) void __stdcall glDisable(int);
__declspec(dllimport) void __stdcall glPushMatrix(void);
__declspec(dllimport) void __stdcall glPopMatrix(void);
__declspec(dllimport) void __stdcall glLoadMatrixf(const float *);
__declspec(dllimport) void __stdcall glLightfv(int, int, const float *);
__declspec(dllimport) void __stdcall glLightModelfv(unsigned, const float *);
}
#pragma intrinsic(sin, cos)

struct BoneView { char head[0x54]; float world[16]; };
struct OwnerView { char head[0x38]; BoneView *bone; };
struct LightView {                    // Light
 void *vtable; OwnerView *owner; int pad[3];
 float ambient[4], diffuse[4], specular[4];   // +0x14 +0x24 +0x34
 float spread_angle;                  // +0x44
 struct V3 { float x, y, z; } position; float type;   // +0x48
 float direction[3];                  // +0x58
 float att_constant, att_linear, att_quad;   // +0x64
};

struct Mat4R {
 float m[16];
 void RotateX_0041fdd0(float degrees);
 void RotateY_0041fe00(float degrees);
 void RotateZ_0041fe30(float degrees);
 void Translate_0041fba0(float x, float y, float z);
};

#define kDegToRad 0.017453292f

void Mat4R::RotateX_0041fdd0(float a) {
 a *= kDegToRad;
 float c = (float)cos(a), s = (float)sin(a);
 m[5] = c; m[6] = s; m[9] = -s; m[10] = c;
}

void Mat4R::RotateY_0041fe00(float a) {
 a *= kDegToRad;
 float c = (float)cos(a), s = (float)sin(a);
 m[0] = c; m[2] = -s; m[8] = s; m[10] = c;
}

void Mat4R::RotateZ_0041fe30(float a) {
 a *= kDegToRad;
 float c = (float)cos(a), s = (float)sin(a);
 m[0] = c; m[1] = s; m[4] = -s; m[5] = c;
}

void Mat4R::Translate_0041fba0(float x, float y, float z) {
 m[12] = x * m[0] + z * m[8] + y * m[4] + m[12];
 m[13] = z * m[9] + y * m[5] + x * m[1] + m[13];
 m[14] = z * m[10] + y * m[6] + x * m[2] + m[14];
 m[15] = z * m[11] + y * m[7] + x * m[3] + m[15];
}

class Renderer_005026d0 {
public:
 char head[0x10];
 int next_light;                      // +0x10, GL_LIGHT0 + used lights
 char body[0xeb020 - 0x14];
 LightView *lights[1000];             // +0xeb020
 int light_count;                     // +0xebfc0
 void AddLight_0041f380(LightView *light);
 void ApplyLights_0041f3c0(void *object);   // object: unused
 void SetAmbient_0041f510(const float *color);
};

void Renderer_005026d0::AddLight_0041f380(LightView *light) {
 lights[light_count] = light;
 light_count++;
}

void Renderer_005026d0::ApplyLights_0041f3c0(void *object) {
 for (int id = 0x4000; id < next_light; id++) glDisable(id);
 next_light = 0x4000;
 for (int i = 0; i < light_count; i++) {
  LightView *light = lights[i];
  glPushMatrix();
  glEnable(next_light);
  glLoadMatrixf(light->owner->bone->world);
  float position[4];
  const LightView::V3 &p = light->position;
  position[0] = p.x;
  position[1] = p.y;
  position[2] = p.z;
  position[3] = light->type;
  glLightfv(next_light, 0x1203, position);              // GL_POSITION
  glLightfv(next_light, 0x1200, light->ambient);
  glLightfv(next_light, 0x1201, light->diffuse);
  glLightfv(next_light, 0x1202, light->specular);
  glLightfv(next_light, 0x1204, light->direction);      // GL_SPOT_DIRECTION
  glLightfv(next_light, 0x1206, &light->spread_angle);  // GL_SPOT_CUTOFF
  glLightfv(next_light, 0x1207, &light->att_constant);
  glLightfv(next_light, 0x1208, &light->att_linear);
  glLightfv(next_light, 0x1208, &light->att_quad);      // sic: GL_LINEAR_ATTENUATION again
  next_light++;
  glPopMatrix();
 }
}

void Renderer_005026d0::SetAmbient_0041f510(const float *color) {
 glLightModelfv(0xb53, color);                         // GL_LIGHT_MODEL_AMBIENT
}
