#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Arabica SAX AttributesImpl<std::string> instantiation (vtable 0x4cd704):
// std::vector of 0x54-byte attributes and its accessors (0x42cbb0-0x42e750).
#pragma warning(disable: 4786)
#include <string>
#include <vector>

struct Attr42 {
 std::string localName;            // 0x00
 std::string uri;                  // 0x10
 std::string qName;                // 0x20
 std::string value;                // 0x30
 std::string type;                 // 0x40
 bool specified;                   // 0x50
};

class SaxAttributes42 {
public:
 virtual int getLength() const = 0;
 virtual int getIndexQ(const std::string &qName) const = 0;
 virtual int getIndex(const std::string &uri, const std::string &localName) const = 0;
 virtual std::string getLocalName_0042d060(unsigned index) const = 0;
 virtual std::string getQName_0042d1a0(unsigned index) const = 0;
 virtual std::string getType_0042d530(const std::string &uri, const std::string &localName) const = 0;
 virtual std::string getType_0042d420(const std::string &qName) const = 0;
 virtual std::string getType_0042d2e0(unsigned index) const = 0;
 virtual std::string getValue_0042d890(const std::string &uri, const std::string &localName) const = 0;
 virtual std::string getValue_0042d780(const std::string &qName) const = 0;
 virtual std::string getValue_0042d640(unsigned index) const = 0;
 virtual std::string getURI_0042d9a0(unsigned index) const = 0;
 virtual ~SaxAttributes42();                         // 0x447630
};

class AttributesImpl_0042cd30 : public SaxAttributes42 {
public:
 std::vector<Attr42> attributes;
 AttributesImpl_0042cd30();
 virtual ~AttributesImpl_0042cd30() {}
 virtual int getLength() const;
 virtual int getIndexQ(const std::string &qName) const;
 virtual int getIndex(const std::string &uri, const std::string &localName) const;
 virtual std::string getLocalName_0042d060(unsigned index) const;
 virtual std::string getQName_0042d1a0(unsigned index) const;
 virtual std::string getType_0042d530(const std::string &uri, const std::string &localName) const;
 virtual std::string getType_0042d420(const std::string &qName) const;
 virtual std::string getType_0042d2e0(unsigned index) const;
 virtual std::string getValue_0042d890(const std::string &uri, const std::string &localName) const;
 virtual std::string getValue_0042d780(const std::string &qName) const;
 virtual std::string getValue_0042d640(unsigned index) const;
 virtual std::string getURI_0042d9a0(unsigned index) const;
 bool isSpecified_0042dae0(int index) const;
 void addAttribute_0042db00(const std::string &uri, const std::string &localName, const std::string &qName,
                            const std::string &type, const std::string &value);
 void addAttribute_0042dea0(const std::string &uri, const std::string &localName, const std::string &qName,
                            const std::string &type, const std::string &value, bool specified);
 const Attr42 *findByQName_0042e140(const std::string &qName) const;
 const Attr42 *findByName_0042e1d0(const std::string &uri, const std::string &localName) const;
};

AttributesImpl_0042cd30::AttributesImpl_0042cd30() {}

int AttributesImpl_0042cd30::getLength() const { return attributes.size(); }

std::string AttributesImpl_0042cd30::getLocalName_0042d060(unsigned index) const { return attributes[index].localName; }
std::string AttributesImpl_0042cd30::getQName_0042d1a0(unsigned index) const { return attributes[index].qName; }
std::string AttributesImpl_0042cd30::getType_0042d2e0(unsigned index) const { return attributes[index].type; }
std::string AttributesImpl_0042cd30::getValue_0042d640(unsigned index) const { return attributes[index].value; }
std::string AttributesImpl_0042cd30::getURI_0042d9a0(unsigned index) const { return attributes[index].uri; }

std::string AttributesImpl_0042cd30::getType_0042d420(const std::string &qName) const {
 const Attr42 *a = findByQName_0042e140(qName);
 if (a != 0) return a->type;
 return std::string();
}

std::string AttributesImpl_0042cd30::getType_0042d530(const std::string &uri, const std::string &localName) const {
 const Attr42 *a = findByName_0042e1d0(uri, localName);
 if (a != 0) return a->type;
 return std::string();
}

std::string AttributesImpl_0042cd30::getValue_0042d780(const std::string &qName) const {
 const Attr42 *a = findByQName_0042e140(qName);
 if (a != 0) return a->value;
 return std::string();
}

std::string AttributesImpl_0042cd30::getValue_0042d890(const std::string &uri, const std::string &localName) const {
 const Attr42 *a = findByName_0042e1d0(uri, localName);
 if (a != 0) return a->value;
 return std::string();
}

int AttributesImpl_0042cd30::getIndexQ(const std::string &qName) const {
 for (unsigned i = 0; i < attributes.size(); i++)
  if (attributes[i].qName == qName) return i;
 return -1;
}

int AttributesImpl_0042cd30::getIndex(const std::string &uri, const std::string &localName) const {
 for (unsigned i = 0; i < attributes.size(); i++)
  if (attributes[i].uri == uri && attributes[i].localName == localName) return i;
 return -1;
}

bool AttributesImpl_0042cd30::isSpecified_0042dae0(int index) const { return attributes[index].specified; }

// NONMATCHING: faithful translation
void AttributesImpl_0042cd30::addAttribute_0042db00(const std::string &uri, const std::string &localName,
                                                    const std::string &qName, const std::string &type,
                                                    const std::string &value) {
 Attr42 &a = *attributes.insert(attributes.end(), Attr42());
 a.uri = uri;
 a.localName = localName;
 a.qName = qName;
 a.value = value;
 a.type = type;
 a.specified = true;
}

// NONMATCHING: faithful translation
void AttributesImpl_0042cd30::addAttribute_0042dea0(const std::string &uri, const std::string &localName,
                                                    const std::string &qName, const std::string &type,
                                                    const std::string &value, bool specified) {
 Attr42 &a = *attributes.insert(attributes.end(), Attr42());
 a.uri = uri;
 a.localName = localName;
 a.qName = qName;
 a.value = value;
 a.type = type;
 a.specified = specified;
}

const Attr42 *AttributesImpl_0042cd30::findByQName_0042e140(const std::string &qName) const {
 for (std::vector<Attr42>::const_iterator i = attributes.begin(); i != attributes.end(); ++i)
  if (i->qName == qName) return &*i;
 return 0;
}

const Attr42 *AttributesImpl_0042cd30::findByName_0042e1d0(const std::string &uri, const std::string &localName) const {
 for (std::vector<Attr42>::const_iterator i = attributes.begin(); i != attributes.end(); ++i)
  if (i->uri == uri && i->localName == localName) return &*i;
 return 0;
}
