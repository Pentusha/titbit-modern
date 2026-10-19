// Bitmap-font text window (vtable 0x4cd4cc, 0x38 bytes): text laid out in
// glyph quads (0x60 bytes each) from a 16x16 font texture whose character
// widths come from a skin record (cache 0x502490). Constructors/destructor:
// lanes/claude/engine_408.cpp (there NamedBuffer_00408a40).
#define E40_COPY_CALL   // the original unit calls the copy ctor 0x402040 (by-value string arguments)
#include "e40_engine.h"
#include "lane_xml.h"
#include "e40_gl.h"
extern "C" {
__declspec(dllimport) char *__stdcall CharLowerA(char *);
__declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
void *__cdecl malloc(unsigned);
void __cdecl free(void *);
}

struct SkinRecord {                   // 0x434 bytes (SoundHolder_0040e380)
 void *vtable;
 char name[0x20];
 int used, refs, age;                 // +0x24
 int widths[256];                     // +0x30 character widths in 1/32 of the char size
 int texture;                         // +0x430
};
/* Out-of-line string helpers the original calls in 0x408d60 (it inlines the
 * skin-cache Load 0x40a300 but not these). Same layout as ObservedString. */
struct StringFromText : ObservedString {   // constructed by the out-of-line ObservedString(const char *) 0x409400
 StringFromText(const char *text);
};
struct StringOps {
 char *data;
 void CopyBeforeWrite_004021c0();
 int ReverseFind_00409580(char ch) const;
 StringOps &Assign_004023e0(const ObservedString &other);
};

struct SkinRecordLoad { void *vtable; char body[0x430]; bool Load_0040e3e0(ObservedString file); };

class SkinCache_00502490 {
public:
 int step, count;
 SkinRecord *data;
 int Find_0040a5d0(ObservedString name);
 int FindFree_0040a720();
 bool Reserve_0040a790(int size);
 SkinRecord *At(int index) { return (index >= 0 && index < count) ? &data[index] : 0; }
 SkinRecord *At_0040a6f0(int index);
 int Load_0040a300(ObservedString path, int age);
};
extern SkinCache_00502490 g_00502490;

struct TextureRecord { char name[0x2c]; char ext[0x10]; };
struct TextureCache { TextureRecord *Get_0042c730(int id); }; // original global 0x5ee6f0
extern TextureCache g_005ee6f0;
struct TextTable_005021a8 { ObservedString Translate_0042a6e0(ObservedString key); };
extern TextTable_005021a8 g_005021a8;

struct Glyph {                        // two triangles over four corners
 float corner[4][3];                  // top left, top right, bottom left, bottom right
 float uv[2][6];
};
struct GlyphUV { float c[6]; };

class NamedBuffer_00408a40 : public EngineObject {
public:
 int texture;                         // 0x14
 int font;                            // 0x18 skin cache index
 float width, height, charSize;       // 0x1c, 0x20, 0x24
 ObservedString text;                 // 0x28
 int align;                           // 0x2c 0 left, 1 right, 2 centre
 Glyph *glyphs;                       // 0x30
 int glyphCount;                      // 0x34
 NamedBuffer_00408a40();
 virtual ~NamedBuffer_00408a40();
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int time);
 void Rebuild_00408d20();
 void Load_00408d60(XmlNode_v *node);
 void Layout_00409c30();
 float AddGlyph_00409e90(unsigned char ch, float x, float y);
};

int NamedBuffer_00408a40::GetType() { return 6; }

void NamedBuffer_00408a40::Rebuild_00408d20() {
 if (glyphs) free(glyphs);
 glyphs = (Glyph *)malloc(text.GetLength() * sizeof(Glyph));
 glyphCount = 0;
 Layout_00409c30();
}

void NamedBuffer_00408a40::Load_00408d60(XmlNode_v *node) {
 ObservedString alignment;
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("parent_bone_id"));
  if (a) field8 = atoi(a->Value()->c_str());
  a = element->Attribute(std::string("texture"));
  if (a) {
   /* SkinCache::Load (0x40a300) expanded inline, as in the original */
   StringFromText path(a->Value()->c_str());
   ((StringOps *)&path)->CopyBeforeWrite_004021c0();
   CharLowerA(path.data);
   ObservedString name;
   ((StringOps *)&name)->Assign_004023e0(path.Mid(((StringOps *)&path)->ReverseFind_00409580('\\') + 1));
   ((StringOps *)&name)->Assign_004023e0(name.Mid(0, ((StringOps *)&name)->ReverseFind_00409580('.')));
   SkinCache_00502490 &cache = g_00502490;
   int index = cache.Find_0040a5d0(name);
   if (index == -1) {
    index = cache.FindFree_0040a720();
    if (index == -1) {
     index = cache.count;
     cache.Reserve_0040a790(cache.step + index);
    }
    cache.data[index].used = index;
    if (!((SkinRecordLoad *)&cache.data[index])->Load_0040e3e0(path)) index = -1;
    else {
     cache.data[index].age = 1;
     cache.data[index].refs = 1;
    }
   } else {
    cache.data[index].refs++;
   }
   font = index;
   texture = g_00502490.At_0040a6f0(font)->texture;
  }
  a = element->Attribute(std::string("length"));
  if (a) width = (float)(atof(a->Value()->c_str()) * 0.01f);
  a = element->Attribute(std::string("height"));
  if (a) height = (float)(atof(a->Value()->c_str()) * 0.01f);
  a = element->Attribute(std::string("charsize"));
  if (a) charSize = (float)(atof(a->Value()->c_str()) * 0.01f);
  a = element->Attribute(std::string("text"));
  if (a) {
   text = a->Value()->c_str();
   if (text[text.GetLength() - 1] != ' ') text += " ";
  }
  a = element->Attribute(std::string("align"));
  if (a) {
   align = 0;
   alignment = a->Value()->c_str();
   if (lstrcmpA(alignment, "RIGHT") == 0) align = 1;
   else if (lstrcmpA(alignment, "CENTER") == 0) align = 2;
  }
  if (--element->refs <= 0) element->Destroy(1);
 }
 Rebuild_00408d20();
}

ObservedString NamedBuffer_00408a40::Export() {
 ObservedString xml, value;
 xml += "<text_window ";
 xml += "parent_bone_id=\"";
 value.Format("%i", field8);
 xml += value;
 xml += "\" ";
 TextureRecord *t = g_005ee6f0.Get_0042c730(texture);
 value = t->name;
 value += ".";
 value += t->ext;
 xml += "texture=\"";
 xml += value;
 xml += "\" ";
 xml += "text=\"";
 xml += text;
 xml += "\" ";
 xml += "length=\"";
 value.Format("%0.4f", (double)width);
 xml += value;
 xml += "\" ";
 xml += "height=\"";
 value.Format("%0.4f", (double)height);
 xml += value;
 xml += "\" ";
 xml += "charsize=\"";
 value.Format("%0.4f", (double)charSize);
 xml += value;
 xml += "\" ";
 xml += "align=\"";
 value = "LEFT";
 if (align == 1) value = "RIGHT";
 else if (align == 2) value = "CENTER";
 xml += value;
 xml += "\" ";
 xml += ">";
 xml += "\n\r";
 xml += "</text_window>";
 xml += "\n\r";
 return xml;
}

// Break the text into lines of at most `width` at spaces and emit the glyphs
// of every line with the requested alignment, top down, until `height` is used.
void NamedBuffer_00408a40::Layout_00409c30() {
 SkinRecord *f = g_00502490.At(font);
 float y = 0;
 int length = text.GetLength();
 if (length == 0) return;
 if (text[length - 1] != ' ') text += " ";
 int i = 0, first = 0;
 float x = 0;
 if (i < length) {
  do {
   int n = 0;
   float word = 0;
   for (int j = i; text[j] != ' ' && j != length; j++) {
    n++;
    word = f->widths[(unsigned char)text[j]] * 0.03125f * charSize + word;
   }
   if (text[n + i] == ' ') {
    n++;
    word = f->widths[' '] * 0.03125f * charSize + word;
   }
   x = word + x;
   i += n;
   if (width < x) {
    i += -1 - n;
    x -= f->widths[' '] * 0.03125f * charSize + word;
    if (align == 1) x = width - x;
    else if (align == 2) x = (width - x) * 0.5f;
    else x = 0;
    for (; first < i; first++) x = AddGlyph_00409e90(text[first], x, y) + x;
    i++;
    y = charSize * 0.8 + y;
    first = i;
    x = 0;
    if (y < -height) return;
   }
  } while (i < length);
  x -= f->widths[' '] * 0.03125f * charSize;
  if (align == 1) x = width - x;
  else if (align == 2) x = (width - x) * 0.5f;
  else x = 0;
  for (; first < i - 1; first++) x = AddGlyph_00409e90(text[first], x, y) + x;
 }
}

float NamedBuffer_00408a40::AddGlyph_00409e90(unsigned char ch, float x, float y) {
 SkinRecord *f = g_00502490.At(font);
 float size = charSize;
 float advance = (float)f->widths[ch] * 0.03125f * charSize;
 float right = x + advance;
 float top = size + y;
 Glyph *g = &glyphs[glyphCount];
 g->corner[0][0] = x; g->corner[0][1] = top; g->corner[0][2] = 0;
 g->corner[1][0] = right; g->corner[1][1] = top; g->corner[1][2] = 0;
 g->corner[2][0] = x; g->corner[2][1] = y; g->corner[2][2] = 0;
 g->corner[3][0] = right; g->corner[3][1] = y; g->corner[3][2] = 0;
 float u0 = (float)(ch & 15) * 0.0625f;
 float v = (float)(15 - (ch >> 4)) * 0.0625f;
 float v0 = v + 0.002f;
 float u1 = advance / size * 0.0625f + u0;
 float v1 = v + 0.0625f;
 GlyphUV a = { u0, v0, u1, v0, u0, v1 };
 *(GlyphUV *)glyphs[glyphCount].uv[0] = a;
 GlyphUV b = { u1, v0, u0, v1, u1, v1 };
 *(GlyphUV *)glyphs[glyphCount].uv[1] = b;
 glyphCount++;
 return advance;
}

// TextWindowCreate script function
NamedBuffer_00408a40 *__cdecl CreateTextWindow_0040a070(const char *text, const char *font, float x, float y, float z,
                                                        float width, float height, float unused, int charSize, int align) {
 NamedBuffer_00408a40 *w = new NamedBuffer_00408a40;
 w->charSize = (float)charSize;
 w->align = align;
 w->width = width;
 w->height = height;
 w->text = g_005021a8.Translate_0042a6e0(text);
 w->font = g_00502490.Load_0040a300(font, -1);
 w->texture = g_00502490.At(w->font)->texture;
 w->Rebuild_00408d20();
 return w;
}

struct LabelBone { char head[0x54]; float matrix[16]; char tail[0xd8 - 0x94]; };
struct LabelOwner { char head[0x38]; LabelBone *bones; };
struct Corner { float x, y, z; };
struct GlyphQuad { Corner topLeft, topRight, bottomLeft, bottomRight; float uv[12]; };   // Glyph by corner

// Draw the glyph quads (two textured triangles each) blended, in the space of
// the parent bone.
void NamedBuffer_00408a40::Update(int time) {
 glEnable(0xde1);                     // GL_TEXTURE_2D
 glBlendFunc(0x302, 0x303);           // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
 glPushMatrix();
 glMultMatrixf(((LabelOwner *)owner)->bones[field8].matrix);
 glBindTexture(0xde1, texture);
 glBegin(4);                          // GL_TRIANGLES
 for (int i = 0; i < glyphCount; i++) {
  Corner topLeft = ((GlyphQuad *)glyphs)[i].topLeft;
  Corner topRight = ((GlyphQuad *)glyphs)[i].topRight;
  Corner bottomLeft = ((GlyphQuad *)glyphs)[i].bottomLeft;
  Corner bottomRight = ((GlyphQuad *)glyphs)[i].bottomRight;
  glTexCoord2f(glyphs[i].uv[0][0], glyphs[i].uv[0][1]);
  glVertex3f(topLeft.x, topLeft.y, topLeft.z);
  glTexCoord2f(glyphs[i].uv[0][2], glyphs[i].uv[0][3]);
  glVertex3f(topRight.x, topRight.y, topRight.z);
  glTexCoord2f(glyphs[i].uv[0][4], glyphs[i].uv[0][5]);
  glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
  glTexCoord2f(glyphs[i].uv[1][0], glyphs[i].uv[1][1]);
  glVertex3f(topRight.x, topRight.y, topRight.z);
  glTexCoord2f(glyphs[i].uv[1][4], glyphs[i].uv[1][5]);
  glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
  glTexCoord2f(glyphs[i].uv[1][2], glyphs[i].uv[1][3]);
  glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
 }
 glEnd();
 glPopMatrix();
 glDisable(0xde1);
 if (next) next->Update(time);
}
