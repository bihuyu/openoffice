/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef _XMLOFF_SCHEMARESTRICTIONCONTEXT_HXX
#define _XMLOFF_SCHEMARESTRICTIONCONTEXT_HXX


//
// include for parent class and members
//

#include "TokenContext.hxx"
#include <com/sun/star/uno/Reference.hxx>


//
// forward declarations
//

namespace com { namespace sun { namespace star {
    namespace xml { namespace sax { class XAttributeList; } }
    namespace beans { class XPropertySet; }
    namespace xforms { class XDataTypeRepository; }
} } }
namespace rtl { class OUString; }
class SvXMLImport;
class SvXMLImportContext;

/** import the xsd:restriction element */
class SchemaRestrictionContext : public TokenContext
{
    com::sun::star::uno::Reference<com::sun::star::xforms::XDataTypeRepository> mxRepository;
    com::sun::star::uno::Reference<com::sun::star::beans::XPropertySet> mxDataType;
    ::rtl::OUString msTypeName;
    ::rtl::OUString msBaseName;

public:

    SchemaRestrictionContext( SvXMLImport& rImport, 
                              sal_uInt16 nPrfx,
                              const ::rtl::OUString& rLName,
                              com::sun::star::uno::Reference<com::sun::star::xforms::XDataTypeRepository>& rRepository,
                              const ::rtl::OUString& sTypeName );
    virtual ~SchemaRestrictionContext();


protected:

    // create mxDataType (if not already present)
    void CreateDataType();

    //
    // implement TokenContext methods:
    //

    virtual void HandleAttribute( 
        sal_uInt16 nToken, 
        const rtl::OUString& rValue );

    virtual SvXMLImportContext* HandleChild( 
        sal_uInt16 nToken,
        sal_uInt16 nPrefix,
        const rtl::OUString& rLocalName,
        const com::sun::star::uno::Reference<com::sun::star::xml::sax::XAttributeList>& xAttrList );
};

#endif
