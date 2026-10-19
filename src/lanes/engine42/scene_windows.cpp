#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Interface window factories: text window (0x424bb0) and textured interface plane (0x425150).
#include "e42_scene.h"
#include <math.h>
extern "C" char *__cdecl strcpy(char *, const char *);
extern "C" int __cdecl strcmp(const char *, const char *);
#pragma intrinsic(strcpy, strcmp)

class GameObject_0040e630 : public SceneObject {
 unsigned char body[0x88 - sizeof(SceneObject)];
public:
 bool selectable;
 GameObject_0040e630();
 void AddComponent_0041e520(void *component);
 bool BindScript_0040e720(ObservedString source);
};
struct Bone_0040c6d0 { SceneMesh mesh; unsigned char rest[0xd8 - sizeof(SceneMesh)]; Bone_0040c6d0(); ~Bone_0040c6d0(); };  // dtor 0x4051b0 (new[] keeps a count)
struct ObjectList42 : ScenePtrArray<SceneObject *> { void Add_00411d70(SceneObject *window); };
struct NameString42 : ObservedString {
 NameString42 &operator=(const char *s) { ObservedString::operator=(s); return *this; }
 int Replace_0040eff0(const char *from, const char *to);
};
struct TextTable42 { ObservedString Translate_0042a6e0(ObservedString key); };
struct FontEntry42 { unsigned char body[0x430]; int height; };
struct FontManager42 {
 int Load_0040a300(ObservedString name, int flags);
 FontEntry42 *Get_0040a6f0(int id);
};
extern FontManager42 g_00502490;
struct TextureManager42 { int Load_0042c2b0(ObservedString name, int flags); };
extern TextureManager42 g_005ee6f0;

class TextWindow_00408b80 {
public:
 void *vtable;
 unsigned reserved4[4];
 int fontHeight;               // 0x14
 int font;                     // 0x18
 float width, height;          // 0x1c
 float size;                   // 0x24
 ObservedString text;          // 0x28
 int color;                    // 0x2c
 unsigned reserved30[2];
 TextWindow_00408b80(GameObject_0040e630 *owner);
 void Layout_00408d20();
};
class InterfacePlane_0040f420 {
public:
 void *vtable;
 unsigned reserved4[4];
 int texture;                  // 0x14
 Vec3_00406f00 size;           // 0x18
 int frame, time, state;       // 0x24
 Vec3_00406f00 uv0, uv1;       // 0x30
 unsigned reserved48[4];
 InterfacePlane_0040f420(GameObject_0040e630 *owner);
};

// NONMATCHING: faithful translation
SceneObject *Scene::CreateTextWindow_00424bb0(SceneObject *parent, const char *text, int x, int y,
                                                       int width, int height, int size, int color,
                                                       const char *script) {
 GameObject_0040e630 *object = new GameObject_0040e630;
 object->reserved4 = (unsigned)parent;
 NameString42 name;
 name = script;
 name.Replace_0040eff0(".scv", "");
 strcpy(object->name, name);
 TextWindow_00408b80 *window = new TextWindow_00408b80(object);
 object->shape = (void *)1;
 Bone_0040c6d0 *bone = new Bone_0040c6d0[1];
 object->mesh = &bone->mesh;
 SceneMesh *mesh = object->mesh;
 mesh->oz = -30.0f;
 mesh->ox = (float)x;
 mesh->oy = (float)y;
 ((int *)object)[0x10] = 1;
 if (strcmp(script, "") != 0) object->selectable = true;
 mesh = object->mesh;
 mesh->depth = 0;
 float w = (float)width;
 float halfWidth = w * 0.5f;
 mesh->width = halfWidth;
 float halfHeight = (float)height * 0.5f;
 mesh->height = halfHeight;
 mesh = object->mesh;
 mesh->z = 0;
 mesh->x = halfWidth;
 mesh->y = halfHeight;
 mesh = object->mesh;
 mesh->radius = (float)sqrt(mesh->depth * mesh->depth + mesh->height * mesh->height + mesh->width * mesh->width);
 object->AddComponent_0041e520(window);
 if (parent == 0) ((ObjectList42 *)&windows)->Add_00411d70(object);
 else ((GameObject_0040e630 *)parent)->AddComponent_0041e520(object);
 object->BindScript_0040e720(ObservedString(script));
 window->color = color;
 window->size = (float)size;
 window->width = w;
 window->height = (float)height;
 window->text = ((TextTable42 *)&pairs)->Translate_0042a6e0(ObservedString(text));
 window->font = g_00502490.Load_0040a300(ObservedString("font"), 1);
 window->fontHeight = g_00502490.Get_0040a6f0(window->font)->height;
 window->Layout_00408d20();
 return object;
}

// NONMATCHING: faithful translation
SceneObject *Scene::CreateIFWindow_00425150(SceneObject *parent, Vec3_00406f00 pos, Vec3_00406f00 size,
                                                     Vec3_00406f00 uv0, Vec3_00406f00 uv1, const char *texture,
                                                     const char *script) {
 GameObject_0040e630 *object = new GameObject_0040e630;
 NameString42 name;
 name = script;
 name.Replace_0040eff0(".scv", "");
 strcpy(object->name, name);
 InterfacePlane_0040f420 *plane = new InterfacePlane_0040f420(object);
 object->reserved4 = (unsigned)parent;
 object->shape = (void *)1;
 Bone_0040c6d0 *bone = new Bone_0040c6d0[1];
 object->mesh = &bone->mesh;
 SceneMesh *mesh = object->mesh;
 mesh->ox = pos.x;
 mesh->oy = pos.y;
 mesh->oz = pos.z;
 ((int *)object)[0x10] = 1;
 object->selectable = true;
 mesh = object->mesh;
 float halfHeight = size.y * 0.5f;
 float halfWidth = size.x * 0.5;
 mesh->width = halfWidth;
 mesh->depth = 0;
 mesh->height = halfHeight;
 mesh = object->mesh;
 mesh->z = 0;
 mesh->x = halfWidth;
 mesh->y = halfHeight;
 mesh = object->mesh;
 mesh->radius = (float)sqrt(mesh->depth * mesh->depth + mesh->height * mesh->height + mesh->width * mesh->width);
 object->AddComponent_0041e520(plane);
 if (parent == 0) ((ObjectList42 *)&windows)->Add_00411d70(object);
 else ((GameObject_0040e630 *)parent)->AddComponent_0041e520(object);
 object->BindScript_0040e720(ObservedString(script));
 plane->texture = g_005ee6f0.Load_0042c2b0(ObservedString(texture), 0);
 plane->frame = 0; plane->time = 0; plane->state = 0;
 plane->size.x = size.x; plane->size.y = size.y; plane->size.z = size.z;
 plane->uv0.x = uv0.x; plane->uv0.y = uv0.y; plane->uv0.z = uv0.z;
 plane->uv1.x = uv1.x; plane->uv1.y = uv1.y; plane->uv1.z = uv1.z;
 ObservedPickingMessage message;
 memset(&message, 0, sizeof(message));
 message.event = 3;
 message.selected = object;
 Send_00424780(&message);
 return object;
}
