#ifdef TITBIT_SOURCE_LINK
// Standalone linkage must construct the complete recovered SAX handler graph.
// The old matching probe declares destructor-only interfaces and cannot emit
// callback vtables suitable for a running parser.
#pragma warning(disable:4786)
#include <string>
#include <exception>
#include "../engine41/cpxml43/xmlincl/DOMBuilder.h"
#include <new>
class DomParser_0042f2c0 {
 unsigned char storage[0x68];
public:
 DomParser_0042f2c0();
 ~DomParser_0042f2c0();
 void SetFeature_0042f610(const std::string&,bool);
};
typedef char DOMBuilderSizeMustMatch[(sizeof(CSL::XML::DOMBuilder)==0x68)?1:-1];
DomParser_0042f2c0::DomParser_0042f2c0(){new(this)CSL::XML::DOMBuilder();}
DomParser_0042f2c0::~DomParser_0042f2c0(){((CSL::XML::DOMBuilder*)this)->~DOMBuilder();}
void DomParser_0042f2c0::SetFeature_0042f610(const std::string &name,bool state){((CSL::XML::DOMBuilder*)this)->setFeature(name,state);}
#else
#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Arabica SAX2DOM parser instantiation used by the scene loader (0x42f2c0-0x42f610):
// construction (handler wiring, default features), destruction, setFeature.
#pragma warning(disable: 4786)
#include <string>

std::string __cdecl MakeString_0045adc0(const std::string &text);
extern std::string g_005ee750;      // "http://xml.org/sax/handlers/LexicalHandler"
extern std::string g_005ee72c;      // "namespaces"

struct SaxContentHandler42 { virtual ~SaxContentHandler42(); };   // 0x447670
struct SaxErrorHandler42 { virtual ~SaxErrorHandler42(); };       // 0x447660
struct SaxLexicalHandler42 { virtual ~SaxLexicalHandler42(); };   // 0x4476b0

class SaxReader42 {
public:
 virtual ~SaxReader42();
 virtual void setContentHandler(SaxContentHandler42 &handler);          // +0x04
 virtual void setErrorHandler(SaxErrorHandler42 &handler);              // +0x08
 virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
 virtual void setFeature(const std::string &name, bool value);          // +0x20
 virtual void s9();
 virtual void setProperty(const std::string &name, void *value);        // +0x28
 unsigned char body[0x4c - 4];
 SaxReader42();                                                          // 0x4403c0
};

struct DOMException_00438fd0 {
 unsigned char body[0x20];
 DOMException_00438fd0(int code);
};

class DomParser_0042f2c0 : public SaxContentHandler42, public SaxErrorHandler42, public SaxLexicalHandler42 {
public:
 SaxReader42 parser;                // 0x0c
 void *document;                    // 0x58
 void *current;                     // 0x5c
 void *entity;                      // 0x60
 bool inCDATA;                      // 0x64
 bool inDTD;                        // 0x65
 bool whiteSpace;                   // 0x66 keep white space in element content
 DomParser_0042f2c0();
 virtual ~DomParser_0042f2c0();
 void SetFeature_0042f610(const std::string &name, bool value);
};

// NONMATCHING: faithful translation
DomParser_0042f2c0::DomParser_0042f2c0() : document(0), current(0), entity(0), inCDATA(false) {
 inDTD = true;
 whiteSpace = true;
 parser.setErrorHandler(*this);
 parser.setContentHandler(*this);
 parser.setProperty(g_005ee750, static_cast<SaxLexicalHandler42 *>(this));
 parser.setFeature(MakeString_0045adc0(std::string("http://xml.org/sax/features/namespaces")), true);
 parser.setFeature(MakeString_0045adc0(std::string("http://xml.org/sax/features/namespace-prefixes")), true);
}

DomParser_0042f2c0::~DomParser_0042f2c0() {}

// NONMATCHING: faithful translation
void DomParser_0042f2c0::SetFeature_0042f610(const std::string &name, bool value) {
 if (name.compare(g_005ee72c) == 0) {
  parser.setFeature(MakeString_0045adc0(std::string("http://xml.org/sax/features/namespaces")), value);
 } else if (name.compare(MakeString_0045adc0(std::string("external-general-entities"))) == 0) {
  parser.setFeature(MakeString_0045adc0(std::string("http://xml.org/sax/features/external-general-entities")), value);
 } else if (name.compare(MakeString_0045adc0(std::string("external-parameter-entities"))) == 0) {
  parser.setFeature(MakeString_0045adc0(std::string("http://xml.org/sax/features/external-parameter-entities")), value);
 } else if (name.compare(MakeString_0045adc0(std::string("white-space-in-element-content"))) == 0) {
  whiteSpace = value;
  return;
 } else if (name == MakeString_0045adc0(std::string("use-iso-8859-1"))) {
  parser.setFeature(MakeString_0045adc0(std::string("http://www.cpointc.com/xml/parser/use-iso-8859-1")), value);
 } else {
  bool known = name == MakeString_0045adc0(std::string("validation")) ||
               name == MakeString_0045adc0(std::string("namespace-declarations")) ||
               name == MakeString_0045adc0(std::string("validate-if-cm")) ||
               name == MakeString_0045adc0(std::string("create-entity-ref-nodes")) ||
               name == MakeString_0045adc0(std::string("entity-nodes")) ||
               name == MakeString_0045adc0(std::string("cdata-nodes")) ||
               name == MakeString_0045adc0(std::string("comments")) ||
               name == MakeString_0045adc0(std::string("charset-overrides-xml-encoding"));
  if (!known) throw DOMException_00438fd0(8);   // NOT_FOUND_ERR
  throw DOMException_00438fd0(9);               // NOT_SUPPORTED_ERR
 }
}

#endif
