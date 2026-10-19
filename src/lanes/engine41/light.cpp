// Light source scene object (vtable 0x4cd5ac, 0x70 bytes) and its XML export.
#include "e41_engine.h"
#include "lane_xml.h"

struct LightList { void Add_0041f380(void *light); };
extern LightList g_005026d0;

struct Color4 { float r, g, b, a; };
ObservedString __cdecl WriteVec3_00413220(const Vec3Z *v, ObservedString name);
ObservedString __cdecl WriteColor_004134a0(const Color4 *c, ObservedString name);


class Light : public EngineObject { // original: ctor 0x412140
public:
 Color4 ambient, diffuse, specular;   // +0x14 +0x24 +0x34
 float spread_angle;                  // +0x44
 Vec3Z position;                      // +0x48
 float type;                          // +0x54 (1 = directional)
 Vec3Z direction;                     // +0x58
 float att_constant, att_linear, att_quad;   // +0x64
 Light(EngineObject *owner);
 virtual ~Light();
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 void Load_004121a0(XmlNode_v *node);
};

Vec3V __cdecl ReadVec3_00412fa0(XmlNode_v *node);
Color4 __cdecl ReadColor_00412c90(XmlNode_v *node);

Light::Light(EngineObject *owner_) {
 owner = owner_;
 spread_angle = 180.0f;
 att_constant = 1.0f;
 att_linear = 0;
 att_quad = 0;
 type = 0;
}

void Light::Load_004121a0(XmlNode_v *node) {
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("light_type"));
  if (a) type = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("att_constant"));
  if (a) att_constant = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("att_linear"));
  if (a) att_linear = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("att_quad"));
  if (a) att_quad = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("spread_angle"));
  if (a) spread_angle = (float)atof(a->Value()->c_str());
  if (--element->refs <= 0) element->Destroy(1);
 }
 for (XmlNode_v *child = node->FirstChild(); child; child = child->NextSibling()) {
  if (*child->Name() == "position") *(Vec3V *)&position = ReadVec3_00412fa0(child);
  else if (*child->Name() == "direction") *(Vec3V *)&direction = ReadVec3_00412fa0(child);
  else if (*child->Name() == "ambient") ambient = ReadColor_00412c90(child);
  else if (*child->Name() == "ambient") ambient = ReadColor_00412c90(child);   // sic: checked twice
  else if (*child->Name() == "diffuse") diffuse = ReadColor_00412c90(child);
  else if (*child->Name() == "specular") specular = ReadColor_00412c90(child);
 }
}

ObservedString Light::Export() {
 ObservedString xml, number;
 xml += "<light_source ";
 xml += "light_type=\"";
 number.Format("%.0f", (double)type);
 xml += number;
 xml += "\" ";
 xml += "att_constant=\"";
 number.Format("%.4f", (double)att_constant);
 xml += number;
 xml += "\" ";
 xml += "att_linear=\"";
 number.Format("%.4f", (double)att_linear);
 xml += number;
 xml += "\" ";
 xml += "att_quad=\"";
 number.Format("%.4f", (double)att_quad);
 xml += number;
 xml += "\" ";
 xml += "spread_angle=\"";
 number.Format("%.4f", (double)spread_angle);
 xml += number;
 xml += "\" ";
 xml += ">";
 xml += WriteVec3_00413220(&position, "position");
 if (type == 1.0f) xml += WriteVec3_00413220(&direction, "direction");
 xml += WriteColor_004134a0(&ambient, "ambient");
 xml += WriteColor_004134a0(&diffuse, "diffuse");
 xml += WriteColor_004134a0(&specular, "specular");
 xml += "</light_source>";
 xml += "\n\r";
 return xml;
}

Color4 __cdecl ReadColor_00412c90(XmlNode_v *node) {
 Color4 c;   // original bug: not initialised, missing attributes return stack garbage
 XmlElement_v *element = node->ToElement();
 if (element) {
  XmlAttr_v *a = element->Attribute(std::string("r"));
  if (a) c.r = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("g"));
  if (a) c.g = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("b"));
  if (a) c.b = (float)atof(a->Value()->c_str());
  a = element->Attribute(std::string("a"));
  if (a) c.a = (float)atof(a->Value()->c_str());
  if (--element->refs <= 0) element->Destroy(1);
 }
 return c;
}

Vec3V __cdecl ReadVec3_00412fa0(XmlNode_v *node) {
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

// "<name x="..." y="..." z="..." />\n\r"
ObservedString __cdecl WriteVec3_00413220(const Vec3Z *v, ObservedString name) {
 ObservedString number;
 name = "<" + name;
 name += " ";
 name += "x=\"";
 number.Format("%.4f", (double)v->x);
 name += number;
 name += "\" ";
 name += "y=\"";
 number.Format("%.4f", (double)v->y);
 name += number;
 name += "\" ";
 name += "z=\"";
 number.Format("%.4f", (double)v->z);
 name += number;
 name += "\" ";
 name += "/>";
 name += "\n\r";
 return name;
}

// "<name r="..." g="..." b="..." a="..." />\n\r"
ObservedString __cdecl WriteColor_004134a0(const Color4 *c, ObservedString name) {
 ObservedString number;
 name = "<" + name;
 name += " ";
 name += "r=\"";
 number.Format("%.4f", (double)c->r);
 name += number;
 name += "\" ";
 name += "g=\"";
 number.Format("%.4f", (double)c->g);
 name += number;
 name += "\" ";
 name += "b=\"";
 number.Format("%.4f", (double)c->b);
 name += number;
 name += "\" ";
 name += "a=\"";
 number.Format("%.4f", (double)c->a);
 name += number;
 name += "\" ";
 name += "/>";
 name += "\n\r";
 return name;
}

void Light::Render(int pass) {
 g_005026d0.Add_0041f380(this);
 if (next) next->Render(pass);
}


Light::~Light() {}


// Native 0x413770 and shared forwarding body 0x406230.
int Light::GetType(){return 5;}
void Light::Update(int time){if(next)next->Update(time);}
