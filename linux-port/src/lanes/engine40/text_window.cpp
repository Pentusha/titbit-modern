// Text window (vtable 0x4cd588, 0x58 bytes): a textured quad on a bone of its
// owner. Constructors, destructor, Draw: lanes/claude/engine_40c.cpp
// (there named Texture_0040f3a0).
#include "e40_engine.h"
#include "lane_xml.h"

struct TextureRecord { char name[0x2c]; char ext[0x10]; };
struct TextureCache { // original global 0x5ee6f0
 TextureRecord *Get_0042c730(int id);
 int Load_0042c2b0(ObservedString name, int flags);
};
extern TextureCache g_005ee6f0;
Vec3V __cdecl ReadVec3_00413a20(XmlNode_v *node);
ObservedString __cdecl WriteVec3_00413cc0(const Vec3Z *v, ObservedString name);

class Texture_0040f3a0 : public EngineObject {
public:
 int resource;                      // 0x14
 Vec3Z size;                        // 0x18
 Vec3Z position;                    // 0x24
 Vec3Z uv;                          // 0x30
 Vec3Z uvSize;                      // 0x3c
 Vec3Z color;                       // 0x48
 float alpha;                       // 0x54
 Texture_0040f3a0();
 virtual ~Texture_0040f3a0();
 virtual int GetType();
 virtual ObservedString Export();
 void Load_0040f640(XmlNode_v *node);
};




#pragma inline_depth(4)   // keeps std::string::c_str out of line, as the original unit (0x40a2f0)
void Texture_0040f3a0::Load_0040f640(XmlNode_v *node) {
 XmlElement_v *element = node->ToElement();
 std::string text;
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("parent_bone_id"));
  if (a) field8 = atoi(a->Value()->c_str());
  a = element->Attribute(std::string("texturename"));
  if (a) {
   text = *a->Value();
   resource = g_005ee6f0.Load_0042c2b0(text.c_str(), 0);
  }
  for (XmlNode_v *child = node->FirstChild(); child; child = child->NextSibling()) {
   if (*child->Name() == "position") *(Vec3V *)&position = ReadVec3_00413a20(child);
   else if (*child->Name() == "size") *(Vec3V *)&size = ReadVec3_00413a20(child);
   else if (*child->Name() == "uvposition") *(Vec3V *)&uv = ReadVec3_00413a20(child);
   else if (*child->Name() == "uvsize") *(Vec3V *)&uvSize = ReadVec3_00413a20(child);
  }
  if (--element->refs <= 0) element->Destroy(1);
 }
}

int Texture_0040f3a0::GetType() { return 7; }

ObservedString Texture_0040f3a0::Export() {
 ObservedString xml, text;
 xml += "<text_window ";
 xml += "parent_bone_id=\"";
 text.Format("%i", field8);
 xml += text;
 xml += "\" ";
 TextureRecord *t = g_005ee6f0.Get_0042c730(resource);
 text = t->name;
 text += ".";
 text += t->ext;
 xml += "texturename=\"";
 xml += text;
 xml += "\" ";
 xml += ">";
 xml += "\n\r";
 xml += WriteVec3_00413cc0(&position, "position");
 xml += WriteVec3_00413cc0(&size, "size");
 xml += WriteVec3_00413cc0(&position, "uvposition");   // sic: writes the position
 xml += WriteVec3_00413cc0(&uvSize, "uvsize");
 xml += "\n\r";
 xml += "</text_window>";
 xml += "\n\r";
 return xml;
}

// TEXTWINDOW.Create script function
Texture_0040f3a0 *__cdecl CreateTextWindow_0040fd20(float px, float py, float pz, float sx, float sy, float sz,
                                                     float ux, float uy, float uz, float wx, float wy, float wz,
                                                     const char *texture, float alpha) {
 Texture_0040f3a0 *w = new Texture_0040f3a0;
 w->resource = g_005ee6f0.Load_0042c2b0(texture, 0);
 w->position.x = px; w->position.y = py; w->position.z = pz;
 w->size.x = sx; w->size.y = sy; w->size.z = sz;
 w->uv.x = ux; w->uv.y = uy; w->uv.z = uz;
 w->uvSize.x = wx; w->uvSize.y = wy; w->uvSize.z = wz;
 w->alpha = alpha;
 w->field8 = 0;
 return w;
}
