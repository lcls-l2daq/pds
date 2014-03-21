#ifndef Pds_ConfigDbSql_QueryProcessor_hh
#define Pds_ConfigDbSql_QueryProcessor_hh

//--------------------------------------------------------------------------
// File and Version Information:
// 	    $Id$
//
// Description:
//	    Class QueryProcessor. The class facilitating query processing.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

#include <string>
#include <map>

#include <stdint.h>
#include <mysql/mysql.h>
#include <time.h>

//----------------------
// Base Class Headers --
//----------------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

#include "pds/configsql/WrongParams.hh"
#include "pds/configsql/DatabaseError.hh"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace Pds { class TypeId; }

namespace Pds_ConfigDb {
  namespace Sql {

    /**
     *  The class facilitating query processing.
     *
     * @version $Id$
     *
     * @author Igor Gaponenko
     */
    class QueryProcessor {

    private:

      struct Cell {
        Cell (const char* thePtr, const unsigned long theLen) :
          ptr (thePtr), len (theLen)
        {}
        const char*         ptr ;
        const unsigned long len ;
      } ;

    public:

      /**
       * Constructor
       */
      QueryProcessor (MYSQL* mysql) ;

      /**
       * Destructor
       */
      ~QueryProcessor () throw () ;

      /**
       * Execute the specified query.
       *
       * This is the very first method to be called after the c-tor. The method
       * can be called multiple times.
       */
      void execute (const std::string& sql) throw (DatabaseError) ;

      /**
       * Return the number of rows in a result set returned by the last query.
       */
      const unsigned long num_rows () const throw (DatabaseError) ;

      /**
       * Check if the last query resulted in an empty result set.
       */
      bool empty () const throw (DatabaseError) ;

      /**
       * Move to the cursor next row.
       *
       * Call this method to move to the next row (if any). The method will
       * return "false" if there will be no next row in the set.
       *
       * NOTE: The method must be called for the very first row too!
       */
      bool next_row () throw (DatabaseError) ;

      /**
       * Extract a value from the currently selected row (integer).
       */
      void get (unsigned&          val,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

      /**
       * Extract a value from the currently selected row (time_t).
       */
      void get_time (time_t&            val,
                     const std::string& col_name,
                     const bool         nullIsAllowed=false) throw (WrongParams,
                                                                    DatabaseError) ;
      
      /**
       * Extract a value from the currently selected row (long).
       */
      void get (long&              val,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

      /**
       * Extract a value from the currently selected row (uint64_t).
       */
      void get (uint64_t&          val,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

      /**
       * Extract a value from the currently selected row (uint64_t).
       */
      void get (Pds::TypeId&       val,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

      /**
       * Extract a value from the currently selected row (double).
       */
      void get (double&            val,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;


      /**
       * Extract a value from the currently selected row (string).
       */
      void get (std::string&       str,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

      /**
       * Extract a value from the currently selected row (blob).
       */
      void get (void*              blob,
                int&               blob_len,
                unsigned           max_len,
                const std::string& col_name,
                const bool         nullIsAllowed=false) throw (WrongParams,
                                                               DatabaseError) ;

    private:

      /**
       * Get raw information on a value at a column of the currently selected row
       *
       * The helper method is used by the "get()' methods. It will also check that
       * an operation is requested in a proper context (q query has been executed
       * and a row is selected). Otherwise an exception will be thrown.
       */
      Cell cell (const std::string& col_name) throw (WrongParams) ;

      /**
       * Reset the context of the object.
       *
       * The method will also release the data structures asssociated
       * with a previously executed query (if any).
       */
      void reset () ;

      /**
       * Check if there was a previously made query, and that query succeeded.
       */
      bool ready2process () const ;

    private:

      // Data members

      std::string m_last_sql ;

      MYSQL*     m_mysql ;
      MYSQL_RES* m_res ;
      MYSQL_ROW  m_row ;

      unsigned long m_num_rows ;
      unsigned long m_next_row ;

      std::map<std::string, unsigned int > m_columns ;

      unsigned long* m_lengths ;
    } ;
  };
}

#endif
