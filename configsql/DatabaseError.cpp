//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class DatabaseError...
//
// Author List:
//      Igor Gaponenko
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------

#include "pds/configsql/DatabaseError.hh"

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

DatabaseError::DatabaseError (const std::string& reason) :
    std::exception(),
    m_reason("LogBook::DatabaseError: " + reason)
{ }

//--------------
// Destructor --
//--------------

DatabaseError::~DatabaseError () throw ()
{ }

//-----------
// Methods --
//-----------

const char*
DatabaseError::what () const throw ()
{
    return m_reason.c_str() ;
}

} // namespace LogBook
