//
// SAX2Parser.cpp
//
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
const XMLString CSAX2Parser::FEATURE_USE_ISO_8859_1     = CXMLUtils::AsciiToXML("http://www.cpointc.com/xml/parser/use-iso-8859-1"); // game version (engine41)
const XMLString CSAX2Parser::PROPERTY_DECL_HANDLER      = CXMLUtils::AsciiToXML("http://xml.org/sax/handlers/DeclHandler");
const XMLString CSAX2Parser::PROPERTY_LEXICAL_HANDLER   = CXMLUtils::AsciiToXML("http://xml.org/sax/handlers/LexicalHandler");
const XMLString CSAX2Parser::PROPERTY_DOCUMENT_BASE     = CXMLUtils::AsciiToXML("http://cpointc.com/xml/parser/DocumentBase");
const XMLString CSAX2Parser::PROPERTY_DEFAULT_HANDLER   = CXMLUtils::AsciiToXML("http://cpointc.com/xml/parser/DefaultHandler");


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
	m_pNamespaceHandler        = NULL;
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
	m_pNamespaceHandler        = NULL;
}


CSAX2Parser::~CSAX2Parser()
{
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
	else if (featureId == FEATURE_USE_ISO_8859_1)   // game version (engine41)
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
	else if (featureId == FEATURE_USE_ISO_8859_1)   // game version (engine41)
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
	pParser->IncludeExternalGeneralEntities(m_featureExtGenEntities);
	pParser->IncludeExternalParameterEntities(m_featureExtParEntities);
	pParser->SetEntityResolver(m_pEntityResolver);
	pParser->SetDTDHandler(m_pDTDHandler);
	pParser->SetContentHandler(m_pContentHandler);
	pParser->SetErrorHandler(m_pErrorHandler);
	pParser->SetLexicalHandler(m_pLexicalHandler);
	pParser->SetDeclHandler(m_pDeclHandler);
	pParser->SetDefaultHandler(m_pDefaultHandler);

	return pParser;
}


XML_END
