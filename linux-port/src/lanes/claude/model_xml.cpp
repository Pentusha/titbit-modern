// Model setup from its XML description: ":bbox_size" child with x/y/z attributes
// (0x4057f0, 0x4058c0). Uses the historical VC6 std::string.
#include "lane_xml.h"
#include "lane_engine.h"

// NONMATCHING 0x4058c0: same size, local float slots/scheduling differ
Vec3V __stdcall ReadVec3_004058c0(XmlNode_v *node) {
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

class BoxedModel_004057f0 : public Node_00405160 {
public:
 char body[0x2b4 - sizeof(Node_00405160)];
 float sx, sy, sz;
 void SetBoxSize_00405320(float x, float y, float z);
 void LoadBox_004057f0(XmlNode_v *node);
};

// NONMATCHING 0x4057f0: name compare inlines differently (9 bytes shorter)
void BoxedModel_004057f0::LoadBox_004057f0(XmlNode_v *node) {
 for (XmlNode_v *child = node->FirstChild(); child; child = child->NextSibling()) {
  if (*child->Name() == ":bbox_size") {
   Vec3V v = ReadVec3_004058c0(child);
   sx = v.x; sy = v.y; sz = v.z;
   SetBoxSize_00405320(v.x, sy, sz);
  }
 }
}
