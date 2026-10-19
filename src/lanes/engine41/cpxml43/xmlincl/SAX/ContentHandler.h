//
// ContentHandler.h
//
// $Id: //depot/XML/Main/include/SAX/ContentHandler.h#2 $
//
// SAX2 ContentHandler Interface.
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
// Copyright (c) 1998-2001 CenterPoint - Connective Software
// Engineering GmbH. All Rights Reserved.
//
// Contributor(s):
//


#ifndef SAX_ContentHandler_H_INCLUDED
#define SAX_ContentHandler_H_INCLUDED


#ifndef XMLStd_H_INCLUDED
#include "XMLStd.h"
#endif
#ifndef XMLString_H_INCLUDED
#include "XMLString.h"
#endif


XML_BEGIN


class CLocator;
class CAttributes;


class XPP_API CContentHandler
	//: Receive notification of the logical content of a document. 
	//+ This is the main interface that most SAX applications implement: 
	//+ if the application needs to be informed of basic parsing events,
	//+ it implements this interface and registers an instance with the SAX 
	//+ parser using the setContentHandler method. The parser
	//+ uses the instance to report basic document-related events like 
	//+ the start and end of elements and character data.
	//+ The order of events in this interface is very important, and mirrors 
	//+ the order of information in the document itself. For example,
	//+ all of an element's content (character data, processing instructions, 
	//+ and/or subelements) will appear, in order, between the
	//+ startElement event and the corresponding endElement event.
	//+ This interface is similar to the now-deprecated SAX 1.0 DocumentHandler 
	//+ interface, but it adds support for Namespaces and
	//. for reporting skipped entities (in non-validating XML processors).
{
public:
	virtual void SetDocumentLocator(const CLocator& loc) = 0;
		//; Receive an object for locating the origin of SAX document events.

	virtual void StartDocument() = 0;
		//; Receive notification of the beginning of a document.

	virtual void EndDocument() = 0;
		//; Receive notification of the end of a document.

	virtual void StartElement(const XMLString& uri, const XMLString& localName, const XMLString& qname, const CAttributes& attrList) = 0;
		//; Receive notification of the beginning of an element.

	virtual void EndElement(const XMLString& uri, const XMLString& localName, const XMLString& qname) = 0;
		//; Receive notification of the end of an element.

	virtual void Characters(const XMLChar ch[], int start, int length) = 0;
		//; Receive notification of character data.

	virtual void IgnorableWhitespace(const XMLChar ch[], int start, int length) = 0;
		//; Receive notification of ignorable whitespace in element content.

	virtual void ProcessingInstruction(const XMLString& target, const XMLString& data) = 0;
		//; Receive notification of a processing instruction.

	virtual void StartPrefixMapping(const XMLString& prefix, const XMLString& uri) = 0;
		//; Begin the scope of a prefix-URI Namespace mapping.

	virtual void EndPrefixMapping(const XMLString& prefix) = 0;
		//; End the scope of a prefix-URI mapping.

	virtual void SkippedEntity(const XMLString& name) = 0;
		//; Receive notification of a skipped entity.

	virtual ~CContentHandler();
};


XML_END


#endif // SAX_ContentHandler_H_INCLUDED
