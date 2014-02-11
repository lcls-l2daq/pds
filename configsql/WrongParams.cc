//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class WrongParams...
//
// Author List:
//      Igor Gaponenko
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------

#include "pds/configsql/WrongParams.hh"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

using namespace Pds_ConfigDb::Sql;

//----------------
// Constructors --
//----------------

WrongParams::WrongParams (const std::string& reason) :
    std::exception(),
    m_reason ("LogBook::WrongParams: "+reason)
{ }

//--------------
// Destructor --
//--------------

WrongParams::~WrongParams () throw ()
{ }

//-----------
// Methods --
//-----------

const char*
WrongParams::what () const throw ()
{
    return m_reason.c_str() ;
}


