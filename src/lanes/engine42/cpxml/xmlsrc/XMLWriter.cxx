//
// XMLWriter.cpp
//
// $Id: //depot/XML/Main/src/XMLWriter.cpp#6 $
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


#include "XMLWriter.h"
#include "XMLUtils.h"
#include "XMLException.h"
#include "AttributeList.h"
#include "AttributeListImp.h"
#include <stdio.h>


XML_BEGIN


#ifdef XML_UNICODE_WCHAR_T
static const XMLChar amp  = L'&';
static const XMLChar lt   = L'<';
static const XMLChar gt   = L'>';
static const XMLChar quot = L'"';
static const XMLChar apos = L'\'';
static const XMLChar tab  = L'\t';
#else
static const XMLChar amp  = '&';
static const XMLChar lt   = '<';
static const XMLChar gt   = '>';
static const XMLChar quot = '"';
static const XMLChar apos = '\'';
static const XMLChar tab  = '\t';
#endif


CXMLWriter::CXMLWriter(XMLCharOutputStream& str):
            m_str(str)
{
	m_level       = -1;
	m_nElems      = 0;
	m_inCDATA     = false;
	m_dataWritten = false;
	m_prettyPrint = false;
}


CXMLWriter::CXMLWriter(XMLCharOutputStream& str, const XMLString& encoding):
            m_str(str),
			m_encoding(encoding)
{
	m_level       = -1;
	m_nElems      = 0;
	m_inCDATA     = false;
	m_dataWritten = false;
	m_prettyPrint = false;
}


CXMLWriter::~CXMLWriter()
{
}


void CXMLWriter::SetDocumentLocator(const CLocator& loc)
{
}


void CXMLWriter::SetDocumentType(const XMLString& publicId, const XMLString& systemId)
{
	m_publicId = publicId;
	m_systemId = systemId;
}


void CXMLWriter::StartDocument()
{
	if (m_level != -1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "StartDocument() called for the second time");
	}
	m_level  = 0;
	m_nElems = 0;

	// write the XML header
	m_str << CXMLUtils::AsciiToXML("<?xml version=\"1.0\"");
	if (m_encoding.length() > 0)
	{
		m_str << CXMLUtils::AsciiToXML(" encoding=\"") << m_encoding << CXMLUtils::AsciiToXML("\"");
	}
	m_str << CXMLUtils::AsciiToXML("?>") << endl;
	if (!m_publicId.empty())
	{
		m_str << CXMLUtils::AsciiToXML("<!DOCTYPE ") << m_publicId
		      << CXMLUtils::AsciiToXML(" SYSTEM \"") << m_systemId
			  << CXMLUtils::AsciiToXML("\">") << endl;
	}
	CheckStream();
	m_dataWritten = true; // avoids an additional newline when pretty printing is active
}


void CXMLWriter::EndDocument()
{
	if (m_level > 0)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "at least one StartElement() has no matching EndElement()");
	}
	m_nElems = 0;
	m_level  = -1;
	m_str << endl;
}


void CXMLWriter::StartFragment()
{
	if (m_level != -1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "StartFragment() called for the second time");
	}
	m_level  = 1;
	m_nElems = 0;
}


void CXMLWriter::EndFragment()
{
	if (m_level > 1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "at least one StartElement() has no matching EndElement()");
	}
	m_nElems = 0;
	m_level  = -1;
	m_str << endl;
}


void CXMLWriter::StartElement(const XMLString& name)
{
	if (m_level == 0 && m_nElems > 1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "StartElement() called for more than one root element");
	}
	PrettyPrint();
	m_level++;
	m_nElems++;
	m_str << CXMLUtils::AsciiToXML("<") << name << CXMLUtils::AsciiToXML(">");
	CheckStream();
	m_elemStack.push_back(name);
	m_dataWritten = false;
}


void CXMLWriter::StartElement(const XMLString& name, const CAttributeList& attrList)
{
	if (m_level == 0 && m_nElems > 1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "More than one root element");
	}
	PrettyPrint();
	m_level++;
	m_nElems++;
	m_str << CXMLUtils::AsciiToXML("<") << name;
	for (int i = 0; i < attrList.GetLength(); i++)
	{
		m_str << CXMLUtils::AsciiToXML(" ") << attrList.GetName(i) << CXMLUtils::AsciiToXML("=\"");
		Characters(attrList.GetValue(i));
		m_str << CXMLUtils::AsciiToXML("\"");
	}
	m_str << CXMLUtils::AsciiToXML(">");
	CheckStream();
	m_elemStack.push_back(name);
	m_dataWritten = false;
}


void CXMLWriter::EndElement(const XMLString& name)
{
	if (m_level == 0)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "EndElement() has no matching StartElement()");
	}
	if (name != m_elemStack.back())
	{
		throw CXMLException(CXMLException::EXMLTagMismatch);
	}
	m_elemStack.pop_back();
	m_level--;
	PrettyPrint();
	m_str << CXMLUtils::AsciiToXML("</") << name << CXMLUtils::AsciiToXML(">");
	if (m_level == 0)
	{
		m_str << endl;
	}
	CheckStream();
	m_dataWritten = false;
}


void CXMLWriter::EmptyElement(const XMLString& name)
{
	CAttributeListImp attr;
	EmptyElement(name, attr);
}


void CXMLWriter::EmptyElement(const XMLString& name, const CAttributeList& attrList)
{
	if (m_level == 0 && m_nElems > 1)
	{
		throw CXMLException(CXMLException::EXMLSyntax, "More than one root element");
	}
	PrettyPrint();
	m_nElems++;
	m_str << CXMLUtils::AsciiToXML("<") << name;
	for (int i = 0; i < attrList.GetLength(); i++)
	{
		m_str << CXMLUtils::AsciiToXML(" ") << attrList.GetName(i) << CXMLUtils::AsciiToXML("=\"");
		Characters(attrList.GetValue(i));
		m_str << CXMLUtils::AsciiToXML("\"");
	}
	m_str << CXMLUtils::AsciiToXML("/>");
	CheckStream();
	m_dataWritten = false;
}


void CXMLWriter::Characters(const XMLChar ch[], int start, int length)
{
	if (m_inCDATA)
	{
		for (int i = 0; i < length; i++, start++)
		{
			m_str << ch[start];
		}
	}
	else
	{
		for (int i = 0; i < length; i++, start++)
		{
			if (ch[start] == quot)
				m_str << CXMLUtils::AsciiToXML("&quot;");
			else if (ch[start] == apos)
				m_str << CXMLUtils::AsciiToXML("&apos;");
			else if (ch[start] == amp)
				m_str << CXMLUtils::AsciiToXML("&amp;");
			else if (ch[start] == lt)
				m_str << CXMLUtils::AsciiToXML("&lt;");
			else if (ch[start] == gt)
				m_str << CXMLUtils::AsciiToXML("&gt;");
			else if ((ch[start] < (XMLChar) 32 || ch[start] > (XMLChar) 126)
			      && ch[start] != (XMLChar) 10 && ch[start] != (XMLChar) 13 && ch[start] != (XMLChar) 9)
			{
				char buffer[32];
#ifdef XML_UNICODE
				sprintf(buffer, "&#x%X;", (unsigned) ch[start]);
#else
				sprintf(buffer, "&#x%X;", (unsigned) (unsigned char) ch[start]);
#endif
				m_str << CXMLUtils::AsciiToXML(buffer);
			}
			else
				m_str << ch[start];
		}
	}
	CheckStream();
	m_dataWritten = true;
}


void CXMLWriter::Characters(const XMLString& str)
{
	Characters(str.c_str(), 0, str.length());
}


void CXMLWriter::IgnorableWhitespace(const XMLChar ch[], int start, int length)
{
	Characters(ch, start, length);
}


void CXMLWriter::ProcessingInstruction(const XMLString& target, const XMLString& data)
{
	PrettyPrint();
	m_str << CXMLUtils::AsciiToXML("<?") << target;
	if (data.length() > 0) m_str << CXMLUtils::AsciiToXML(" ") << data;
	m_str << CXMLUtils::AsciiToXML("?>");
	if (m_level == 0) m_str << endl;
	CheckStream();
}


void CXMLWriter::DataElement(const XMLString& name, const XMLString& data, 
	                         const XMLString& att1, const XMLString& val1,
							 const XMLString& att2, const XMLString& val2,
							 const XMLString& att3, const XMLString& val3)
{
	CAttributeListImp attr;
	if (!att1.empty()) attr.AddAttribute(att1, CXMLUtils::AsciiToXML("CDATA"), val1);
	if (!att2.empty()) attr.AddAttribute(att2, CXMLUtils::AsciiToXML("CDATA"), val2);
	if (!att3.empty()) attr.AddAttribute(att3, CXMLUtils::AsciiToXML("CDATA"), val3);
	if (data.empty())
	{
		EmptyElement(name, attr);
	}
	else
	{
		StartElement(name, attr);
		Characters(data);
		EndElement(name);
	}
}


void CXMLWriter::StartCDATA()
{
	m_inCDATA = true;
	m_str << CXMLUtils::AsciiToXML("<![CDATA[");
	CheckStream();
}


void CXMLWriter::EndCDATA()
{
	m_inCDATA = false;
	m_str << CXMLUtils::AsciiToXML("]]>");
	CheckStream();
}


void CXMLWriter::Comment(const XMLChar ch[], int start, int length)
{
	PrettyPrint();
	m_str << CXMLUtils::AsciiToXML("<!--");
	for (int i = 0; i < length; i++, start++)
	{
		m_str << ch[start];
	}
	m_str << CXMLUtils::AsciiToXML("-->");
	CheckStream();
	m_dataWritten = false;
}


void CXMLWriter::StartDTD(const XMLString& name, const XMLString& publicId, const XMLString& systemId)
{
}


void CXMLWriter::EndDTD()
{
}


void CXMLWriter::StartEntity(const XMLString& name)
{
}


void CXMLWriter::EndEntity(const XMLString& name)
{
}


void CXMLWriter::PrettyPrint() const
{
	if (m_prettyPrint && !m_dataWritten)
	{
		m_str << endl;
		for (int i = 0; i < m_level; ++i) m_str << tab;
	}
	CheckStream();
}


void CXMLWriter::SetPrettyPrint(bool enable)
{
	m_prettyPrint = enable;
}


void CXMLWriter::CheckStream() const
{
	if (m_str.bad()) throw CXMLException(CXMLException::EXMLIOError);
}


XML_END
