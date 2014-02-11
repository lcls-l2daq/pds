//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class ValueTypeMismatch...
//
// Author List:
//      Igor Gaponenko
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------

#include "pds/configsql/ValueTypeMismatch.hh"

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

ValueTypeMismatch::ValueTypeMismatch (const std::string& reason) :
    std::exception(),
    m_reason ("LogBook::ValueTypeMismatch: "+reason)
{ }

//--------------
// Destructor --
//--------------

ValueTypeMismatch::~ValueTypeMismatch () throw ()
{ }

//-----------
// Methods --
//-----------

const char*
ValueTypeMismatch::what () const throw ()
{
    return m_reason.c_str() ;
}

