//
// SAX2Parser.cpp
//
// Modified for the game revision (use-iso-8859-1 feature, Latin-1 handler) by the engine42 lane.
// $Id: //depot/XML/Main/src/SAX2Parser.cpp#4 $
//
// The contents of this file are subject to the 
// CenterPoint Public License Version 1.0 (the "License"); 
// you may not use this file except in compliance with the 
// License. You may obtain a copy of the License at 
// http://www.cpointc.com/CPL
//
// Software distributed under the License is distributed on 
// an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the License for the specific 
// language governing rights and limitations under the License.
//
// The Original Code is CenterPoint/XML.
//
// The Initial Developer of the Original Code is CenterPoint - 
// Connective Software Engineering GmbH. Portions created by
// CenterPoint - Connective Software Engineering GmbH. are 
// Copyright (c) 1998-2000 CenterPoint - Connective Software
// Engineering GmbH. All Rights Reserved.
//
// Contributor(s):
//


#include "SAX2Parser.h"
#include "SAX2ParserBase.h"
#include "SAX2ParserNS.h"
#include "SAX2ParserNSP.h"
#include "EntityResolver.h"
#include "DTDHandler.h"
#include "ContentHandler.h"
#include "AnyHandler.h"
#include "AttributeListImp.h"
#include "AttributesImpl.h"
#include "InputSource.h"
#include "ErrorHandler.h"
#include "XMLException.h"
#include "SAXException.h"
#include "SAXParseException.h"
#include "SAXNotSupportedException.h"
#include "SAXNotRecognizedException.h"
#include "XMLUtils.h"
#include <fstream>


#ifdef CSL_XML_STANDALONE
#define PARSE_EXCEPTION CXMLException
#else
#define PARSE_EXCEPTION CSL::Base::CException
#endif


XML_BEGIN


const XMLString CSAX2Parser::FEATURE_VALIDATION         = CXMLUtils::AsciiToXML("http://xml.org/sax/features/validation");
const XMLString CSAX2Parser::FEATURE_NAMESPACES         = CXMLUtils::AsciiToXML("http://xml.org/sax/features/namespaces");
const XMLString CSAX2Parser::FEATURE_NAMESPACE_PREFIXES = CXMLUtils::AsciiToXML("http://xml.org/sax/features/namespace-prefixes");
const XMLString CSAX2Parser::FEATURE_EXT_GEN_ENTITIES   = CXMLUtils::AsciiToXML("http://xml.org/sax/features/external-general-entities");
const XMLString CSAX2Parser::FEATURE_EXT_PAR_ENTITIES   = CXMLUtils::AsciiToXML("http://xml.org/sax/features/external-parameter-entities");
const XMLString CSAX2Parser::FEATURE_STRING_INTERNING   = CXMLUtils::AsciiToXML("http://xml.org/sax/features/string-interning");
const XMLString CSAX2Parser::FEATURE_USE_ISO_8859_1     = CXMLUtils::AsciiToXML("http://www.cpointc.com/xml/parser/use-iso-8859-1"); // game revision
const XMLString CSAX2Parser::PROPERTY_DECL_HANDLER      = CXMLUtils::AsciiToXML("http://xml.org/sax/handlers/DeclHandler");
const XMLString CSAX2Parser::PROPERTY_LEXICAL_HANDLER   = CXMLUtils::AsciiToXML("http://xml.org/sax/handlers/LexicalHandler");
const XMLString CSAX2Parser::PROPERTY_DOCUMENT_BASE     = CXMLUtils::AsciiToXML("http://cpointc.com/xml/parser/DocumentBase");
const XMLString CSAX2Parser::PROPERTY_DEFAULT_HANDLER   = CXMLUtils::AsciiToXML("http://cpointc.com/xml/parser/DefaultHandler");


// Game revision: content handler wrapper that converts UTF-8 names and text to
// ISO-8859-1 (characters above U+00FF become 'X') before forwarding them.
class CLatin1ContentHandler: public CContentHandler
{
public:
	CLatin1ContentHandler() { m_pHandler = NULL; }
	// (no user-declared destructor: the implicit one is a bare jmp to ~CContentHandler, 0x443200)

	void Convert(const XMLChar* s, int start, int length, XMLString& out)
	{
		unsigned int c = 0;
		int n = 0;
		if (length > 0)
		{
			const XMLChar* p = s + start;
			do
			{
				unsigned int b = (unsigned char) *p;
				if (b <= 0x7f)
				{
					out += (char) b;
					c = 0;
					n = 0;
				}
				else if ((b & 0xfc) == 0xfc) { c = b & 0x01; n = 5; }
				else if ((b & 0xf8) == 0xf8) { c = b & 0x03; n = 4; }
				else if ((b & 0xf0) == 0xf0) { c = b & 0x07; n = 3; }
				else if ((b & 0xe0) == 0xe0) { c = b & 0x0f; n = 2; }
				else if ((b & 0xc0) == 0xc0) { c = b & 0x1f; n = 1; }
				else
				{
					b = (b & 0x3f) | (c << 6);
					c = b;
					if (--n == 0)
					{
						if (c > 0xff)
							b = 'X';
						out += (char) b;
					}
				}
				++p;
			} while (--length);
		}
	}

	void SetDocumentLocator(const CLocator& loc) { m_pHandler->SetDocumentLocator(loc); }
	void StartDocument() { m_pHandler->StartDocument(); }
	void EndDocument() { m_pHandler->EndDocument(); }

	void StartElement(const XMLString& uri, const XMLString& localName, const XMLString& qname, const CAttributes& attrList)
	{
		CAttributesImpl attrs;
		int n = attrList.GetLength();
		for (int i = 0; i < n; ++i)
		{
			XMLString u, l, q, v;
			XMLString s = attrList.GetURI(i);       Convert(s.c_str(), 0, s.size(), u);
			XMLString t = attrList.GetLocalName(i); Convert(t.c_str(), 0, t.size(), l);
			XMLString w = attrList.GetQName(i);     Convert(w.c_str(), 0, w.size(), q);
			XMLString x = attrList.GetValue(i);     Convert(x.c_str(), 0, x.size(), v);
			attrs.AddAttribute(u, l, q, attrList.GetType(i), v);
		}
		XMLString cu, cl, cq;
		Convert(uri.c_str(), 0, uri.size(), cu);
		Convert(localName.c_str(), 0, localName.size(), cl);
		Convert(qname.c_str(), 0, qname.size(), cq);
		m_pHandler->StartElement(cu, cl, cq, attrs);
	}

	void EndElement(const XMLString& uri, const XMLString& localName, const XMLString& qname)
	{
		XMLString cu, cl, cq;
		Convert(uri.c_str(), 0, uri.size(), cu);
		Convert(localName.c_str(), 0, localName.size(), cl);
		Convert(qname.c_str(), 0, qname.size(), cq);
		m_pHandler->EndElement(cu, cl, cq);
	}

	void Characters(const XMLChar ch[], int start, int length)
	{
		XMLString s;
		Convert(ch, start, length, s);
		m_pHandler->Characters(s.c_str(), 0, s.size());
	}

	void IgnorableWhitespace(const XMLChar ch[], int start, int length)
	{
		m_pHandler->IgnorableWhitespace(ch, start, length);
	}

	void ProcessingInstruction(const XMLString& target, const XMLString& data)
	{
		XMLString ct, cd;
		Convert(target.c_str(), 0, target.size(), ct);
		Convert(data.c_str(), 0, data.size(), cd);
		m_pHandler->ProcessingInstruction(ct, cd);
	}

	void StartPrefixMapping(const XMLString& prefix, const XMLString& uri)
	{
		XMLString cp, cu;
		Convert(prefix.c_str(), 0, prefix.size(), cp);
		Convert(uri.c_str(), 0, uri.size(), cu);
		m_pHandler->StartPrefixMapping(cp, cu);
	}

	void EndPrefixMapping(const XMLString& prefix)
	{
		XMLString cp;
		Convert(prefix.c_str(), 0, prefix.size(), cp);
		m_pHandler->EndPrefixMapping(cp);
	}

	void SkippedEntity(const XMLString& name)
	{
		XMLString cn;
		Convert(name.c_str(), 0, name.size(), cn);
		m_pHandler->SkippedEntity(cn);
	}

	CContentHandler* m_pHandler;
};



CSAX2Parser::CSAX2Parser()
{
	m_featureNamespaces        = true;
	m_featureNamespacePrefixes = false;
	m_featureExtGenEntities    = true;
	m_featureExtParEntities    = true;
	m_featureUseIso88591       = false;
	m_pEntityResolver          = NULL;
	m_pDTDHandler              = NULL;
	m_pContentHandler          = NULL;
	m_pErrorHandler            = NULL;
	m_pLexicalHandler          = NULL;
	m_pDeclHandler             = NULL;
	m_pDefaultHandler          = NULL;
	m_pLatin1Handler           = NULL;
}


CSAX2Parser::CSAX2Parser(const XMLString& encoding)
{
	m_encoding                 = encoding;
	m_featureNamespaces        = true;
	m_featureNamespacePrefixes = false;
	m_featureExtGenEntities    = true;
	m_featureExtParEntities    = true;
	m_featureUseIso88591       = false;
	m_pEntityResolver          = NULL;
	m_pDTDHandler              = NULL;
	m_pContentHandler          = NULL;
	m_pErrorHandler            = NULL;
	m_pLexicalHandler          = NULL;
	m_pDeclHandler             = NULL;
	m_pDefaultHandler          = NULL;
	m_pLatin1Handler           = NULL;
}


CSAX2Parser::~CSAX2Parser()
{
	delete m_pLatin1Handler;
}


void CSAX2Parser::SetEntityResolver(CEntityResolver* pResolver)
{
	m_pEntityResolver = pResolver;
}


void CSAX2Parser::SetDTDHandler(CDTDHandler* pDTDHandler)
{
	m_pDTDHandler = pDTDHandler;
}


void CSAX2Parser::SetContentHandler(CContentHandler* pContentHandler)
{
	m_pContentHandler = pContentHandler;
}


void CSAX2Parser::SetErrorHandler(CErrorHandler* pErrorHandler)
{
	m_pErrorHandler = pErrorHandler;
}


CEntityResolver* CSAX2Parser::GetEntityResolver() const
{
	return m_pEntityResolver;
}


CDTDHandler* CSAX2Parser::GetDTDHandler() const
{
	return m_pDTDHandler;
}


CContentHandler* CSAX2Parser::GetContentHandler() const
{
	return m_pContentHandler;
}


CErrorHandler* CSAX2Parser::GetErrorHandler() const
{
	return m_pErrorHandler;
}


void CSAX2Parser::Parse(CInputSource* pSource)
{
	assertion (pSource);

	CSAX2ParserBase* pParser = MakeParser(pSource->GetEncoding());
	pParser->SetBase(pSource->GetSystemId());
	try
	{
		if (pSource->GetCharacterStream())
		{
			pParser->ParseChar(pSource->GetCharacterStream());
		}
		else if (pSource->GetByteStream())
		{
			pParser->Parse(pSource->GetByteStream());
		}
		else
		{
			throw CSAXParseException("no input stream", XMLString(), pSource->GetSystemId(), 0, 0, CXMLException(CXMLException::EXMLBadInputSource, ""));
		}
	}
	catch (PARSE_EXCEPTION&)
	{
		delete pParser;
		throw;
	}
	delete pParser;
}


// Game revision
void CSAX2Parser::ParseMemory(const char* xml)
{
	CSAX2ParserBase* pParser = MakeParser(m_encoding);
	pParser->SetBase(XMLString("memory"));
	try
	{
		pParser->ParseMemory(xml);
	}
	catch (PARSE_EXCEPTION&)
	{
		delete pParser;
		throw;
	}
	delete pParser;
}


void CSAX2Parser::Parse(const XMLString& systemId)
{
	// Since we do not know better,
	// we create a byte input stream.
	ifstream file;
	file.open(CXMLUtils::XMLToAscii(systemId).c_str(), ios::in);
	if (file.good())
	{
		CInputSource source;
		source.SetByteStream(&file);
		source.SetSystemId(systemId);
		source.SetEncoding(m_encoding);
		CSAX2ParserBase* pParser = MakeParser(m_encoding);
		pParser->SetInputSource(&source);
		pParser->SetBase(systemId);
		try
		{
			pParser->Parse(&file);
		}
		catch (PARSE_EXCEPTION&)
		{
			delete pParser;
			file.close();
			throw;
		}
		delete pParser;
		file.close();
	}
	else
	{
#ifdef CSL_XML_STANDALONE
		throw CSAXParseException("file cannot be opened", XMLString(), CXMLUtils::XMLToAscii(systemId), 0, 0, CXMLException(CXMLException::EXMLBadSystemId, string()));
#else
		throw CSL::Base::CException(CException::EOpenFile, CXMLUtils::XMLToAscii(systemId));
#endif
	}
}


void CSAX2Parser::SetFeature(const XMLString& featureId, bool state)
{
	if (featureId == FEATURE_VALIDATION || featureId == FEATURE_STRING_INTERNING)
	{
		throw CSAXNotSupportedException(CXMLUtils::XMLToAscii(FEATURE_VALIDATION));
	}
	else if (featureId == FEATURE_EXT_GEN_ENTITIES)
	{
		m_featureExtGenEntities = state;
	}
	else if (featureId == FEATURE_EXT_PAR_ENTITIES)
	{
		m_featureExtParEntities = state;
	}
	else if (featureId == FEATURE_NAMESPACES)
	{
		m_featureNamespaces = state;
	}
	else if (featureId == FEATURE_NAMESPACE_PREFIXES)
	{
		m_featureNamespacePrefixes = state;
	}
	else if (featureId == FEATURE_USE_ISO_8859_1)
	{
		m_featureUseIso88591 = state;
	}
	else
	{
		throw CSAXNotRecognizedException(CXMLUtils::XMLToAscii(featureId));
	}
}


bool CSAX2Parser::GetFeature(const XMLString& featureId) const
{
	if (featureId == FEATURE_VALIDATION || featureId == FEATURE_STRING_INTERNING)
	{
		throw CSAXNotSupportedException(CXMLUtils::XMLToAscii(FEATURE_VALIDATION));
	}
	else if (featureId == FEATURE_EXT_GEN_ENTITIES)
	{
		return m_featureExtGenEntities;
	}
	else if (featureId == FEATURE_EXT_PAR_ENTITIES)
	{
		return m_featureExtParEntities;
	}
	else if (featureId == FEATURE_NAMESPACES)
	{
		return m_featureNamespaces;
	}
	else if (featureId == FEATURE_NAMESPACE_PREFIXES)
	{
		return m_featureNamespacePrefixes;
	}
	else if (featureId == FEATURE_USE_ISO_8859_1)
	{
		return m_featureUseIso88591;
	}
	else
	{
		throw CSAXNotRecognizedException(CXMLUtils::XMLToAscii(featureId));
	}
}


void CSAX2Parser::SetProperty(const XMLString& propertyId, const XMLString& value)
{
	if (propertyId == PROPERTY_DOCUMENT_BASE)
	{
		m_documentBase = value;
	}
	else if (propertyId == PROPERTY_DECL_HANDLER || propertyId == PROPERTY_LEXICAL_HANDLER)
	{
		throw CSAXNotSupportedException(string("property does not take a string value: ") + CXMLUtils::XMLToAscii(propertyId));
	}
	else
	{
		throw CSAXNotRecognizedException(CXMLUtils::XMLToAscii(propertyId));
	}
}


void CSAX2Parser::SetProperty(const XMLString& propertyId, void* value)
{
	if (propertyId == PROPERTY_DOCUMENT_BASE)
	{
		throw CSAXNotSupportedException(string("property does not take a pointer value: ") + CXMLUtils::XMLToAscii(propertyId));
	}
	else if (propertyId == PROPERTY_DECL_HANDLER)
	{
		m_pDeclHandler = reinterpret_cast<CDeclHandler*>(value);
	}
	else if (propertyId == PROPERTY_LEXICAL_HANDLER)
	{
		m_pLexicalHandler = reinterpret_cast<CLexicalHandler*>(value);
	}
	else if (propertyId == PROPERTY_DEFAULT_HANDLER)
	{
		m_pDefaultHandler = reinterpret_cast<CAnyHandler*>(value);
	}
	else
	{
		throw CSAXNotRecognizedException(CXMLUtils::XMLToAscii(propertyId));
	}
}


void* CSAX2Parser::GetProperty(const XMLString& propertyId) const
{
	if (propertyId == PROPERTY_DOCUMENT_BASE)
	{
		return (void*) m_documentBase.c_str();
	}
	else if (propertyId == PROPERTY_DECL_HANDLER)
	{
		return m_pDeclHandler;
	}
	else if (propertyId == PROPERTY_LEXICAL_HANDLER)
	{
		return m_pLexicalHandler;
	}
	else if (propertyId == PROPERTY_DEFAULT_HANDLER)
	{
		return m_pDefaultHandler;
	}
	else
	{
		throw CSAXNotSupportedException(CXMLUtils::XMLToAscii(propertyId));
	}
}


CSAX2ParserBase* CSAX2Parser::MakeParser(const XMLString& encoding)
{
	CSAX2ParserBase* pParser = NULL;
	if (m_featureNamespaces && m_featureNamespacePrefixes)
	{
		pParser = new CSAX2ParserNSP(encoding.empty() ? NULL : encoding.c_str());
	}
	else if (m_featureNamespaces)
	{
		pParser = new CSAX2ParserNS(encoding.empty() ? NULL : encoding.c_str());
	}
	else
	{
		pParser = new CSAX2ParserBase(encoding.empty() ? NULL : encoding.c_str());
	}
	if (m_featureUseIso88591)
	{
		if (!m_pLatin1Handler)
			m_pLatin1Handler = new CLatin1ContentHandler;
		m_pLatin1Handler->m_pHandler = m_pContentHandler;
	}
	pParser->IncludeExternalGeneralEntities(m_featureExtGenEntities);
	pParser->IncludeExternalParameterEntities(m_featureExtParEntities);
	pParser->SetEntityResolver(m_pEntityResolver);
	pParser->SetDTDHandler(m_pDTDHandler);
	pParser->SetContentHandler(m_featureUseIso88591 ? m_pLatin1Handler : m_pContentHandler);
	pParser->SetErrorHandler(m_pErrorHandler);
	pParser->SetLexicalHandler(m_pLexicalHandler);
	pParser->SetDeclHandler(m_pDeclHandler);
	pParser->SetDefaultHandler(m_pDefaultHandler);

	return pParser;
}


XML_END
