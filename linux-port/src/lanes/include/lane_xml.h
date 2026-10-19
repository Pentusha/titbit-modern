// Engine XML node interface (vtable slots as used by the game) and helpers.
#ifndef LANE_XML_H
#define LANE_XML_H
#pragma warning(disable: 4786)
#include <string>
#include <stdlib.h>
struct Vec3V { float x, y, z; Vec3V() {} Vec3V(float a, float b, float c) { x = a; y = b; z = c; } };

struct XmlAttr_v {
 virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
 virtual const std::string *Value();                     // +0x18
};
struct XmlElement_v {
 virtual void s0();
 virtual void Destroy(int);                              // +0x04
 virtual XmlAttr_v *Attribute(const std::string &name);  // +0x08
 int refs;
};
struct XmlNode_v {
 virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
 virtual const std::string *Name();                      // +0x14
 virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
 virtual XmlNode_v *FirstChild();                        // +0x2c
 virtual void s12(); virtual void s13();
 virtual XmlNode_v *NextSibling();                       // +0x38
 virtual XmlElement_v *ToElement();                      // +0x3c
};

#endif
