/*
 * Copyright (c) 2011-2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "XmlDocSource.h"
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <stdlib.h>

using std::string;

// Schedule for libxml2 library
bool CXmlDocSource::_bLibXml2CleanupScheduled;

CXmlDocSource::CXmlDocSource(_xmlDoc *pDoc, _xmlNode *pRootNode,
                             bool bValidateWithSchema) :
      _pDoc(pDoc),
      _pRootNode(pRootNode),
      _strXmlSchemaFile(""),
      _strRootElementType(""),
      _strRootElementName(""),
      _strNameAttrituteName(""),
      _bNameCheck(false),
      _bValidateWithSchema(bValidateWithSchema)
{
    init();
}

CXmlDocSource::CXmlDocSource(_xmlDoc *pDoc,
                             const string& strXmlSchemaFile,
                             const string& strRootElementType,
                             const string& strRootElementName,
                             const string& strNameAttrituteName) :
    _pDoc(pDoc),
    _pRootNode(NULL),
    _strXmlSchemaFile(strXmlSchemaFile),
    _strRootElementType(strRootElementType),
    _strRootElementName(strRootElementName),
    _strNameAttrituteName(strNameAttrituteName),
    _bNameCheck(true),
    _bValidateWithSchema(false)
{
    init();
}

CXmlDocSource::CXmlDocSource(_xmlDoc* pDoc,
                             const string& strXmlSchemaFile,
                             const string& strRootElementType,
                             bool bValidateWithSchema) :
    _pDoc(pDoc), _pRootNode(NULL),
    _strXmlSchemaFile(strXmlSchemaFile),
    _strRootElementType(strRootElementType),
    _strRootElementName(""),
    _strNameAttrituteName(""),
    _bNameCheck(false),
    _bValidateWithSchema(bValidateWithSchema)
{
    init();
}

CXmlDocSource::CXmlDocSource(_xmlDoc *pDoc,
                             const string& strXmlSchemaFile,
                             const string& strRootElementType,
                             const string& strRootElementName,
                             const string& strNameAttrituteName,
                             bool bValidateWithSchema) :
    _pDoc(pDoc),
    _pRootNode(NULL),
    _strXmlSchemaFile(strXmlSchemaFile),
    _strRootElementType(strRootElementType),
    _strRootElementName(strRootElementName),
    _strNameAttrituteName(strNameAttrituteName),
    _bNameCheck(true),
    _bValidateWithSchema(bValidateWithSchema)
{
    init();
}

CXmlDocSource::~CXmlDocSource()
{
    if (_pDoc) {
        // Free XML doc
        xmlFreeDoc(_pDoc);
        _pDoc = NULL;
    }
}

void CXmlDocSource::getRootElement(CXmlElement& xmlRootElement) const
{
    xmlRootElement.setXmlElement(_pRootNode);
}

string CXmlDocSource::getRootElementName() const
{
    return (const char*)_pRootNode->name;
}

string CXmlDocSource::getRootElementAttributeString(const string& strAttributeName) const
{
    CXmlElement topMostElement(_pRootNode);

    return topMostElement.getAttributeString(strAttributeName);
}

_xmlDoc* CXmlDocSource::getDoc() const
{
    return _pDoc;
}

bool CXmlDocSource::validate(CXmlSerializingContext& serializingContext)
{
    // Check that the doc has been created
    if (!_pDoc) {

        serializingContext.setError("Could not parse document ");

        return false;
    }

    // Validate if necessary
    if (_bValidateWithSchema)
    {
        if (!isInstanceDocumentValid()) {

            serializingContext.setError("Document is not valid");

            return false;
        }
    }

    // Check Root element type
    if (getRootElementName() != _strRootElementType) {

        serializingContext.setError("Error: Wrong XML structure document ");
        serializingContext.appendLineToError("Root Element " + getRootElementName()
                                             + " mismatches expected type " + _strRootElementType);

        return false;
    }

    if (_bNameCheck) {

        string strRootElementNameCheck = getRootElementAttributeString(_strNameAttrituteName);

        // Check Root element name attribute (if any)
        if (!_strRootElementName.empty() && strRootElementNameCheck != _strRootElementName) {

            serializingContext.setError("Error: Wrong XML structure document ");
            serializingContext.appendLineToError(_strRootElementType + " element "
                                                 + _strRootElementName + " mismatches expected "
                                                 + _strRootElementType + " type "
                                                 + strRootElementNameCheck);

            return false;
        }
    }

    return true;
}

void CXmlDocSource::init()
{
    if (!_bLibXml2CleanupScheduled) {

        // Schedule cleanup
        atexit(xmlCleanupParser);

        _bLibXml2CleanupScheduled = true;
    }

    if (!_pRootNode) {

        _pRootNode = xmlDocGetRootElement(_pDoc);
    }
}

bool CXmlDocSource::isInstanceDocumentValid()
{
#ifdef LIBXML_SCHEMAS_ENABLED
    xmlDocPtr pSchemaDoc = xmlReadFile(_strXmlSchemaFile.c_str(), NULL, XML_PARSE_NONET);

    if (!pSchemaDoc) {
        // Unable to load Schema
        return false;
    }

    xmlSchemaParserCtxtPtr pParserCtxt = xmlSchemaNewDocParserCtxt(pSchemaDoc);

    if (!pParserCtxt) {

        // Unable to create schema context
        xmlFreeDoc(pSchemaDoc);
        return false;
    }

    // Get Schema
    xmlSchemaPtr pSchema = xmlSchemaParse(pParserCtxt);

    if (!pSchema) {

        // Invalid Schema
        xmlSchemaFreeParserCtxt(pParserCtxt);
        xmlFreeDoc(pSchemaDoc);
        return false;
    }
    xmlSchemaValidCtxtPtr pValidationCtxt = xmlSchemaNewValidCtxt(pSchema);

    if (!pValidationCtxt) {

        // Unable to create validation context
        xmlSchemaFree(pSchema);
        xmlSchemaFreeParserCtxt(pParserCtxt);
        xmlFreeDoc(pSchemaDoc);
        return false;
    }

    xmlSetStructuredErrorFunc(this, schemaValidityStructuredErrorFunc);

    bool isDocValid = xmlSchemaValidateDoc(pValidationCtxt, _pDoc) == 0;

    xmlSchemaFreeValidCtxt(pValidationCtxt);
    xmlSchemaFree(pSchema);
    xmlSchemaFreeParserCtxt(pParserCtxt);
    xmlFreeDoc(pSchemaDoc);

    return isDocValid;
#else
    return true;
#endif
}

void CXmlDocSource::schemaValidityStructuredErrorFunc(void* pUserData, _xmlError* pError)
{
    (void)pUserData;

#ifdef LIBXML_SCHEMAS_ENABLED
    // Display message
    puts(pError->message);
#endif
}
