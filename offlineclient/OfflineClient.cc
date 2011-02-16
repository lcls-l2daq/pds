#include <string.h>
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "OfflineClient.hh"

#include "LogBook/Connection.h"

using namespace Pds;

//
// OfflineClient (current experiment name passed in)
//
OfflineClient::OfflineClient(const char* path, const char* instrument_name, const char* experiment_name) :
    _path (path),
    _instrument_name (instrument_name),
    _experiment_name (experiment_name)
{
  
    bool found = false;
    printf("entered OfflineClient(path=%s, instr=%s, exp=%s)\n", _path, _instrument_name, _experiment_name);

    // translate experiment name to experiment number
    LogBook::Connection * conn = NULL;

    try {
        if (strcmp(path, "/dev/null") == 0) {
            printf("fake it (path=/dev/null)\n");
            _experiment_number = 1;
        } else {
            _experiment_number = 0;
            conn = LogBook::Connection::open(path);
            if (conn == NULL) {
                printf("LogBook::Connection::connect() failed\n");
            }
        }

        if (conn != NULL) {
            // begin transaction
            conn->beginTransaction();

            // get current experiment and experiment list
            std::string instrument = _instrument_name;
            std::vector<LogBook::ExperDescr> experiments;


            conn->getExperiments(experiments, instrument);
            conn->getCurrentExperiment(_experiment_descr, instrument);

            for (size_t ii = 0 ; ii < experiments.size(); ii++) {
                if (experiments[ii].name.compare(_experiment_name) == 0) {
                    _experiment_number = (unsigned int) experiments[ii].id;
                    found = true;
                    break;
                }
            }

            // if experiment name passed in != current experiment in DB, complain
            if (_experiment_name != _experiment_descr.name.c_str()){
              printf("Error:  OfflineClient():  %s/%s does not match current experiment in database, %s\n", 
                     _instrument_name, _experiment_name, _experiment_descr.name.c_str());
            }

        }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }

    if (conn != NULL) {
        // close connection
        delete conn ;
    }

    printf ("OfflineClient(): experiment %s/%s (#%d) \n",
            _instrument_name, _experiment_name, _experiment_number);
}

//
// OfflineClient (current experiment name retrieved from database)
//
OfflineClient::OfflineClient(const char* path, const char* instrument_name) :
    _path (path),
    _instrument_name (instrument_name)
{

    printf("entered OfflineClient(path=%s, instr=%s)\n", _path, _instrument_name);

    // translate experiment name to experiment number
    LogBook::Connection * conn = NULL;

    try {
        if (strcmp(path, "/dev/null") == 0) {
            printf("fake it (path=/dev/null)\n");
            _experiment_number = 1;
        } else {
            _experiment_number = 0;
            conn = LogBook::Connection::open(path);
            if (conn == NULL) {
                printf("LogBook::Connection::connect() failed\n");
            }
        }

        if (conn != NULL) {
            // begin transaction
            conn->beginTransaction();

            // get current experiment
            std::string instrument = _instrument_name;
            
            conn->getCurrentExperiment(_experiment_descr, instrument);

            _experiment_name = _experiment_descr.name.c_str();
            _experiment_number = _experiment_descr.id;

        }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }

    if (conn != NULL) {
        // close connection
        delete conn ;
    }

    printf ("OfflineClient(): experiment %s/%s (#%d) \n",
            _instrument_name, _experiment_name, _experiment_number);
}

//
// AllocateRunNumber
//
// Allocate a new run number from the database.
// If database is NULL then the run number is set to 0.
//
// RETURNS: 0 if successful, otherwise -1
//
int OfflineClient::AllocateRunNumber(unsigned int *runNumber) {
  LogBook::Connection * conn = NULL;
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (runNumber && _instrument_name && _experiment_name) {

    // in case of NULL database, set run number to 0
    if ((_path == (char *)NULL) || (strcmp(_path, "/dev/null") == 0)) {
      *runNumber = _run_number = 0;
      returnVal = 0;  // OK
    } else {
      printf("Allocating run number\n");
      try {
        conn = LogBook::Connection::open(_path);

        if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          // allocate run # from database
          _run_number = (unsigned) conn->allocateRunNumber(_instrument_name, _experiment_name);
          *runNumber = _run_number;
          returnVal = 0; // OK

          // commit transaction
          conn->commitTransaction();
        } else {
            printf("LogBook::Connection::connect() failed\n");
        }

      } catch (const LogBook::ValueTypeMismatch& e) {
        printf ("Parameter type mismatch %s:\n", e.what());
        returnVal = -1; // ERROR

      } catch (const LogBook::WrongParams& e) {
        printf ("Problem with parameters %s:\n", e.what());
        returnVal = -1; // ERROR
    
      } catch (const LogBook::DatabaseError& e) {
        printf ("Database operation failed: %s\n", e.what());
        returnVal = -1; // ERROR
      }

      if (0 == returnVal) {
        printf("Completed allocating run number %d\n",_run_number);
      }

      if (conn != NULL) {
        // close connection
        delete conn ;
      }
    }
  }

  if (-1 == returnVal) {
    _run_number = 0;
    if (runNumber)
      *runNumber = _run_number;
    printf("%s returning error, setting run number = 0\n", __FUNCTION__);
  }

  return (returnVal);
}

int OfflineClient::reportOpenFile (int expt, int run, int stream, int chunk) {
  LogBook::Connection * conn = NULL;
  int returnVal = -1;  // default return is ERROR

  // sanity check
  // JBT - may not work (experiment_name, compare to space)
  if (run && _instrument_name && _experiment_name) {

    // in case of NULL database, report nothing
    if ((_path == (char *)NULL) || (strcmp(_path, "/dev/null") == 0)) {
      returnVal = 0;  // OK
    } else {
      try {
        conn = LogBook::Connection::open(_path);

        if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

	  conn->reportOpenFile(expt, run, stream, chunk);
          returnVal = 0; // OK

          // commit transaction
          conn->commitTransaction();
        } else {
            printf("LogBook::Connection::connect() failed\n");
        }

      } catch (const LogBook::ValueTypeMismatch& e) {
        printf ("Parameter type mismatch %s:\n", e.what());
        returnVal = -1; // ERROR

      } catch (const LogBook::WrongParams& e) {
        printf ("Problem with parameters %s:\n", e.what());
        returnVal = -1; // ERROR
    
      } catch (const LogBook::DatabaseError& e) {
        printf ("Database operation failed: %s\n", e.what());
        returnVal = -1; // ERROR
      }

      if (conn != NULL) {
        // close connection
        delete conn ;
      }
    }
  }

  if (-1 == returnVal) {
    printf("Error reporting open file expt %d run %d stream %d chunk %d\n",
	   expt, run, stream, chunk);
  }

  return (returnVal);
}

//
// GetExperimentNumber
//
unsigned int OfflineClient::GetExperimentNumber() {
    return (_experiment_number);
}

//
// GetExperimentName
//
const char * OfflineClient::GetExperimentName() {
  return (_experiment_name);
}

//
// GetInstrumentName
//
const char * OfflineClient::GetInstrumentName() {
    return (_instrument_name);
}

//
// GetPath
//
const char * OfflineClient::GetPath() {
    return (_path);
}

