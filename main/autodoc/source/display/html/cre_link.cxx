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

#include <precomp.h>
#include "cre_link.hxx"


// NOT FULLY DEFINED SERVICES
#include <ary/cpp/c_class.hxx>
#include <ary/cpp/c_define.hxx>
#include <ary/cpp/c_enum.hxx>
#include <ary/cpp/c_enuval.hxx>
#include <ary/cpp/c_funct.hxx>
#include <ary/cpp/c_gate.hxx>
#include <ary/cpp/c_macro.hxx>
#include <ary/cpp/c_namesp.hxx>
#include <ary/cpp/c_tydef.hxx>
#include <ary/cpp/c_vari.hxx>
#include <ary/cpp/cp_ce.hxx>
#include <ary/loc/loc_file.hxx>
#include <ary/loc/locp_le.hxx>
#include "hdimpl.hxx"
#include "opageenv.hxx"
#include "strconst.hxx"





LinkCreator::LinkCreator( char *              o_rOutput,
                          uintt               i_nOutputSize )
    :   pOut(o_rOutput),
        nOutMaxSize(i_nOutputSize),
        pEnv(0)
{
}

LinkCreator::~LinkCreator()
{
}

void
LinkCreator::do_Process( const ary::cpp::Namespace & i_rData )
{
    Create_PrePath( i_rData );
    strcat( pOut, "index.html" );   // KORR_FUTURE   // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Class & i_rData )
{
    Create_PrePath( i_rData );
    strcat( pOut, ClassFileName(i_rData.LocalName().c_str()) ); // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Enum & i_rData )
{
    Create_PrePath( i_rData );
    strcat( pOut, EnumFileName(i_rData.LocalName().c_str()) ); // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Typedef & i_rData )
{
    Create_PrePath( i_rData );
    strcat( pOut, TypedefFileName(i_rData.LocalName().c_str()) ); // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Function & i_rData )
{
    Create_PrePath( i_rData );

    if ( i_rData.Protection() != ary::cpp::PROTECT_global )
    {
        strcat( pOut, "o.html" );   // SAFE STRCAT (#100211# - checked)
    }
    else
    {
        csv_assert(i_rData.Location().IsValid());
        const ary::loc::File &
            rFile = pEnv->Gate().Locations().Find_File(i_rData.Location());
        strcat( pOut, HtmlFileName("o-", rFile.LocalName().c_str()) ); // SAFE STRCAT (#100211# - checked)
    }

    csv_assert(pEnv != 0);
    strcat( pOut, OperationLink(pEnv->Gate(), i_rData.LocalName(), i_rData.CeId()) ); // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Variable & i_rData )
{
    Create_PrePath( i_rData );

    if ( i_rData.Protection() != ary::cpp::PROTECT_global )
    {
        strcat( pOut, "d.html" );       // SAFE STRCAT (#100211# - checked)
    }
    else
    {
        csv_assert(i_rData.Location().IsValid());
        const ary::loc::File &
            rFile = pEnv->Gate().Locations().Find_File(i_rData.Location());
        strcat( pOut, HtmlFileName("d-", rFile.LocalName().c_str()) );  // SAFE STRCAT (#100211# - checked)
    }

    strcat( pOut, DataLink(i_rData.LocalName()) );  // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::EnumValue & i_rData )
{
    const ary::cpp::CodeEntity *
        pEnum = pEnv->Gate().Ces().Search_Ce(i_rData.Owner());
    if (pEnum == 0)
        return;

    pEnum->Accept(*this);
    strcat(pOut, "#");      // SAFE STRCAT (#100211# - checked)
    strcat(pOut, i_rData.LocalName().c_str());  // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Define & i_rData )
{
    // KORR_FUTURE
    // Only valid from Index:

    *pOut = '\0';
    strcat(pOut, "../def-all.html#");               // SAFE STRCAT (#100211# - checked)
    strcat(pOut, i_rData.LocalName().c_str());      // SAFE STRCAT (#100211# - checked)
}

void
LinkCreator::do_Process( const ary::cpp::Macro & i_rData )
{
    // KORR_FUTURE
    // Only valid from Index:

    *pOut = '\0';
    strcat(pOut, "../def-all.html#");               // SAFE STRCAT (#100211# - checked)
    strcat(pOut, i_rData.LocalName().c_str());    // SAFE STRCAT (#100211# - checked)
}


namespace
{

class NameScope_const_iterator
{
  public:
                        NameScope_const_iterator(
                            ary::cpp::Ce_id     i_nId,
                            const ary::cpp::Gate &
                                                i_rGate );

                        operator bool() const   { return pCe != 0; }
    const String  &     operator*() const;

    void                go_up();

  private:
    const ary::cpp::CodeEntity *
                        pCe;
    const ary::cpp::Gate *
                        pGate;
};


NameScope_const_iterator::NameScope_const_iterator(
                                        ary::cpp::Ce_id         i_nId,
                                        const ary::cpp::Gate &  i_rGate )
    :   pCe(i_rGate.Ces().Search_Ce(i_nId)),
        pGate(&i_rGate)
{
}

const String  &
NameScope_const_iterator::operator*() const
{
 	return pCe ? pCe->LocalName()
               : String::Null_();
}

void
NameScope_const_iterator::go_up()
{
 	if (pCe == 0)
        return;
    pCe = pGate->Ces().Search_Ce(pCe->Owner());
}


void                Recursive_CreatePath(
                        char *              o_pOut,
                        const NameScope_const_iterator &
                                            i_it        );

void
Recursive_CreatePath( char *                            o_pOut,
                      const NameScope_const_iterator &  i_it        )
{
    if (NOT i_it)
        return;

    NameScope_const_iterator it( i_it );
    it.go_up();
    if (NOT it)
        return;     // Global Namespace
    Recursive_CreatePath( o_pOut, it );

    strcat( o_pOut, (*i_it).c_str() );          // SAFE STRCAT (#100211# - checked)
    strcat( o_pOut, "/" );                      // SAFE STRCAT (#100211# - checked)
}


}   // anonymous namespace





void
LinkCreator::Create_PrePath( const ary::cpp::CodeEntity & i_rData )
{
    *pOut = NULCH;

    if ( pEnv->CurNamespace() != 0 )
    {
        if ( pEnv->CurClass()
                ?   pEnv->CurClass()->CeId() == i_rData.Owner()
                :   pEnv->CurNamespace()->CeId() == i_rData.Owner() )
            return;

        strcat( pOut, PathUp(pEnv->Depth() - 1) );      // SAFE STRCAT (#100211# - checked)
    }
    else
    {   // Within Index
        strcat( pOut, "../names/" );                    // SAFE STRCAT (#100211# - checked)
    }

    NameScope_const_iterator it( i_rData.Owner(), pEnv->Gate() );
    Recursive_CreatePath( pOut, it );
}
