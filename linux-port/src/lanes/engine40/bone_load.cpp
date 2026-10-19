// Bone description from XML (0x40d600): ids, radius and the position/center/
// bbox/rotation/scale children. Vector reader 0x40d9c0: lanes/claude/bone_xml.cpp.
#include "lane_xml.h"

Vec3V __stdcall ReadVec3_0040d9c0(XmlNode_v *node);

struct BoneVec { float x, y, z; };
struct Bone_0040c6d0 {                // 0xd8 bytes
 int id;                            // +0
 BoneVec position;                  // +4
 float rotation[4];                 // +0x10
 BoneVec scale;                     // +0x20
 BoneVec center;                    // +0x2c
 BoneVec bbox;                      // +0x38
 BoneVec v44;                       // +0x44
 float radius;                      // +0x50
 float local[16], world[16];        // +0x54, +0x94
 int parent;                        // +0xd4
 void SetRotation_0040ddc0(float x, float y, float z);
 void Load_0040d600(XmlNode_v *node);
};

void Bone_0040c6d0::Load_0040d600(XmlNode_v *node) {
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("bone_id"));
  if (a) id = atoi(a->Value()->c_str());
  a = element->Attribute(std::string("parent_bone_id"));
  if (a) parent = atoi(a->Value()->c_str());
  a = element->Attribute(std::string("radius"));
  if (a) radius = (float)atof(a->Value()->c_str());
  if (--element->refs <= 0) element->Destroy(1);
 }
 for (XmlNode_v *child = node->FirstChild(); child; child = child->NextSibling()) {
  if (*child->Name() == "position") *(Vec3V *)&position = ReadVec3_0040d9c0(child);
  else if (*child->Name() == "center") *(Vec3V *)&center = ReadVec3_0040d9c0(child);
  else if (*child->Name() == "bbox") *(Vec3V *)&bbox = ReadVec3_0040d9c0(child);
  else if (*child->Name() == "rotation") {
   Vec3V r = ReadVec3_0040d9c0(child);
   SetRotation_0040ddc0(r.x, r.y, r.z);
  }
  else if (*child->Name() == "scale") *(Vec3V *)&scale = ReadVec3_0040d9c0(child);
 }
}
