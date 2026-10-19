// Bone setup from XML (0x40d9c0 and neighbours). The vector reader is a second
// copy of the one at 0x4058c0, compiled in this unit.
#include "lane_xml.h"

Vec3V __stdcall ReadVec3_0040d9c0(XmlNode_v *node) {
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
