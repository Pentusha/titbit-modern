// Frame renderer (global at 0x5026d0, ctor 0x41e8f0): depth-sorted list of
// transparent triangles and sprites drawn after the opaque pass.
extern "C" {
__declspec(dllimport) void __stdcall glDepthMask(unsigned char);
__declspec(dllimport) void __stdcall glEnable(unsigned);
__declspec(dllimport) void __stdcall glDisable(unsigned);
__declspec(dllimport) void __stdcall glBlendFunc(unsigned, unsigned);
__declspec(dllimport) void __stdcall glBegin(unsigned);
__declspec(dllimport) void __stdcall glEnd(void);
__declspec(dllimport) void __stdcall glBindTexture(unsigned, unsigned);
__declspec(dllimport) void __stdcall glColor4f(float, float, float, float);
__declspec(dllimport) void __stdcall glTexCoord2f(float, float);
__declspec(dllimport) void __stdcall glVertex3f(float, float, float);
__declspec(dllimport) void __stdcall glVertex3fv(const float *);
}

struct Vec3Z { float x, y, z; Vec3Z() { x = y = z = 0; } };
struct Mat4I {
 float m[16];
 Mat4I() {
  m[0] = m[5] = m[10] = m[15] = 1.0f;
  m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0;
 }
};

struct DepthEntry {                   // 0x60 bytes
 Vec3Z v0, v1, v2;                    // +0x00 (sprite: v0 = corner, v1.x = size)
 float uv[6];                         // +0x24
 float color[4];                      // +0x3c (sprites)
 int type;                            // +0x4c: 0 triangle, 1 sprite, 2 additive sprite
 float depth;                         // +0x50
 unsigned texture;                    // +0x54
 DepthEntry *next;                    // +0x58 (towards nearer, draw order)
 DepthEntry *prev;                    // +0x5c
};

struct Color4 { float r, g, b, a; };
struct UvSet { float uv[6]; };
struct StackEntry { int head[4]; Mat4I matrix; };

class Renderer_0041e8f0 {
public:
 int head[4];
 int next_light;                      // +0x10
 int count;                           // +0x14
 DepthEntry entries[10000];           // +0x18
 DepthEntry *first;                   // +0xea618
 DepthEntry *cursor;                  // +0xea61c
 StackEntry stack[32];                // +0xea620
 void *lights[1000];                  // +0xeb020
 int light_count;                     // +0xebfc0
 Mat4I view;                          // +0xebfc4
 float zoom, fov;                     // +0xec004
 Renderer_0041e8f0();
 void DrawTransparent_0041ea00();
 void AddSprite_0041ec80(const float *position, const float *color, const unsigned *texture, const float *size, bool additive);
 void AddTriangle_0041ef40(const float *a, const float *b, const float *c, const float *uv, const unsigned *texture);
 void Link(float depth);
};

Renderer_0041e8f0::Renderer_0041e8f0() {
 zoom = 1.0f;
 fov = 40.0f;
 light_count = 0;
 next_light = 0x4000;
}

// Insert entries[count] into the depth list, searching from the cursor.
inline void Renderer_0041e8f0::Link(float depth) {
 DepthEntry *entry = &entries[count];
 if (count == 0) {
  cursor = &entries[0];
  entries[0].next = 0;
  cursor->prev = 0;
  first = cursor;
  count++;
  return;
 }
 if (!(depth < cursor->depth)) {          // original test: also taken for unordered (NaN)
  for (;;) {
   DepthEntry *p = cursor->prev;
   if (p == 0) {
    cursor->prev = entry;
    entries[count].next = cursor;
    entries[count].prev = 0;
    cursor = first = &entries[count];
    count++;
    return;
   }
   if (depth < p->depth) break;
   cursor = p;
  }
  DepthEntry *p = cursor->prev;
  cursor->prev = entry;
  entries[count].next = cursor;
  entries[count].prev = p;
  p->next = &entries[count];
 } else {
  DepthEntry *n;
  while ((n = cursor->next) != 0) {
   if (n->depth < depth) {
    n = cursor->next;
    cursor->next = entry;
    entries[count].prev = cursor;
    entries[count].next = n;
    n->prev = &entries[count];
    cursor = &entries[count];
    count++;
    return;
   }
   cursor = n;
  }
  cursor->next = entry;
  entries[count].prev = cursor;
  entries[count].next = 0;
 }
 cursor = &entries[count];
 count++;
}

void Renderer_0041e8f0::DrawTransparent_0041ea00() {
 int texture = -1;
 glDepthMask(0);
 glDisable(0xb44);
 glEnable(0xbc0);
 glEnable(0xbe2);
 glEnable(0xde1);
 glBlendFunc(0x302, 0x303);
 glBegin(4);
 while (first) {
  if (texture != (int)first->texture) {
   glEnd();
   texture = first->texture;
   glBindTexture(0xde1, texture);
   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
   glBlendFunc(0x302, first->type == 1 ? 1 : 0x303);
   glBegin(4);
  }
  DepthEntry *e = first;
  if (e->type == 0) {
   glTexCoord2f(e->uv[0], e->uv[1]);
   glVertex3f(first->v0.x, first->v0.y, first->v0.z);
   glTexCoord2f(first->uv[2], first->uv[3]);
   glVertex3f(first->v1.x, first->v1.y, first->v1.z);
   glTexCoord2f(first->uv[4], first->uv[5]);
   glVertex3f(first->v2.x, first->v2.y, first->v2.z);
  } else {
   glColor4f(e->color[0], e->color[1], e->color[2], e->color[3]);
   float p[3];
   p[0] = first->v0.x;
   p[1] = first->v0.y;
   p[2] = first->v0.z;
   float size = first->v1.x;
   glTexCoord2f(0, 0);
   glVertex3f(p[0], p[1] - size, p[2]);
   glTexCoord2f(1.0f, 0);
   glVertex3f(p[0] + size, p[1] - size, p[2]);
   glTexCoord2f(0, 1.0f);
   glVertex3fv(p);
   glVertex3fv(p);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(p[0] + size, p[1], p[2]);
   glTexCoord2f(1.0f, 0);
   glVertex3f(p[0] + size, p[1] - size, p[2]);
  }
  first = first->next;
 }
 glEnd();
 glDepthMask(1);
 glDisable(0xde1);
 glDisable(0xbc0);
 glDisable(0xbe2);
}

void Renderer_0041e8f0::AddSprite_0041ec80(const float *position, const float *color, const unsigned *texture, const float *size, bool additive) {
 entries[count].v0.x = position[0] - *size * 0.5;
 entries[count].v0.y = *size * 0.5 + position[1];
 entries[count].v0.z = position[2];
 entries[count].v1.x = *size;
 *(Color4 *)entries[count].color = *(const Color4 *)color;
 entries[count].texture = *texture;
 entries[count].type = additive ? 2 : 1;
 entries[count].depth = position[2] * -3.0f;
 Link(entries[count].depth);
}

void Renderer_0041e8f0::AddTriangle_0041ef40(const float *a, const float *b, const float *c, const float *uv, const unsigned *texture) {
 DepthEntry &e = entries[count];
 e.v0.x = a[0]; e.v0.y = a[1]; e.v0.z = a[2];
 e.v1.x = b[0]; e.v1.y = b[1]; e.v1.z = b[2];
 e.v2.x = c[0]; e.v2.y = c[1]; e.v2.z = c[2];
 *(UvSet *)e.uv = *(const UvSet *)uv;
 e.v0.x = a[0]; e.v0.y = a[1]; e.v0.z = a[2];         // sic: stored twice
 e.v1.x = b[0]; e.v1.y = b[1]; e.v1.z = b[2];
 e.v2.x = c[0]; e.v2.y = c[1]; e.v2.z = c[2];
 *(UvSet *)e.uv = *(const UvSet *)uv;
 entries[count].texture = *texture;
 entries[count].type = 0;
 entries[count].depth = -(a[2] + c[2] + b[2]);
 Link(entries[count].depth);
}
