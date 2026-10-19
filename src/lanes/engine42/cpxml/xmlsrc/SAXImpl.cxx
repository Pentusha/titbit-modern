//
// SAXImpl.cpp
//
// $Id: //depot/XML/Main/src/SAXImpl.cpp#4 $
//
// Provides a home for virtual destructors of SAX interfaces
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


#include "AttributeList.h"
#include "Attributes.h"
#include "Configurable.h"
#include "DefaultHandler.h"
#include "AnyHandler.h"
#include "DocumentHandler.h"
#include "ContentHandler.h"
#include "DTDHandler.h"
#include "EntityResolver.h"
#include "ErrorHandler.h"
#include "LexicalHandler.h"
#include "Locator.h"
#include "NamespaceHandler.h"
#include "Parser.h"
#include "XMLReader.h"


XML_BEGIN


CAttributeList::~CAttributeList()
{
}


CAttributes::~CAttributes()
{
}


CConfigurable::~CConfigurable()
{
}


CAnyHandler::~CAnyHandler()
{
}


CDocumentHandler::~CDocumentHandler()
{
}


CContentHandler::~CContentHandler()
{
}


CDTDHandler::~CDTDHandler()
{
}


CEntityResolver::~CEntityResolver()
{
}


CErrorHandler::~CErrorHandler()
{
}


CLexicalHandler::~CLexicalHandler()
{
}


CLocator::~CLocator()
{
}


CNamespaceHandler::~CNamespaceHandler()
{
}


CParser::~CParser()
{
}


CXMLReader::~CXMLReader()
{
}


XML_END

