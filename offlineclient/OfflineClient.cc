#include <string.h>
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "OfflineClient.hh"

#include "LogBook/Connection.h"

using namespace Pds;

//
// OfflineDbBeginRun
//
class OfflineDbBeginRun : public Action {
public:

  OfflineDbBeginRun(OfflineClient* offlineclient) :
    _offlineclient (offlineclient)
  {
  }

  InDatagram* fire(InDatagram* dg) {
    return dg;
  }

  Transition* fire(Transition* tr) {
    LogBook::Connection * conn = NULL;
    LusiTime::Time now;

    printf("Storing BeginRun logbook information\n");
    try {
      conn = LogBook::Connection::open(_offlineclient->GetPath());

      if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          // retrieve run # that was allocated earlier
	  RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
          _run_number = rinfo.run();

          // begin run
          now = LusiTime::Time::now();
          conn->beginRun(_offlineclient->GetInstrumentName(),
                         _offlineclient->GetExperimentName(),
                         _run_number, "DATA", now); // TODO DATA/CALIB
          // commit transaction
          conn->commitTransaction();
      } else {
          // TODO improve error handling
          printf("LogBook::Connection::connect() failed\n");
      }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }
    printf("Completed storing BeginRun logbook information\n");

    if (conn != NULL) {
      // close connection
      delete conn ;
    }

    return tr;
  }
private:
  OfflineClient* _offlineclient;
  unsigned _run_number;
};

//
// OfflineNullBeginRun
//
class OfflineNullBeginRun : public Action {
public:
  OfflineNullBeginRun() {}

  InDatagram* fire(InDatagram* dg) {
    return dg;
  }

  Transition* fire(Transition* tr) {
    // null database
    return tr;
  }
};

//
// OfflineDbEndRun
//
class OfflineDbEndRun : public Action {
public:
  OfflineDbEndRun(OfflineClient* offlineclient) :
    _offlineclient (offlineclient)
  {
  }
  InDatagram* fire(InDatagram* dg) {
    return dg;
  }
  Transition* fire(Transition* tr) {
    LogBook::Connection * conn = NULL;
    LusiTime::Time now;

    printf("Storing EndRun logbook information\n");
    try {
      conn = LogBook::Connection::open(_offlineclient->GetPath());

      if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          // end run
          now = LusiTime::Time::now();
          conn->endRun(_offlineclient->GetInstrumentName(),
                         _offlineclient->GetExperimentName(),
                         _offlineclient->GetRunNumber(), now);
          // commit transaction
          conn->commitTransaction();
      } else {
          // TODO improve error handling
          printf("LogBook::Connection::connect() failed\n");
      }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }
    printf("Completed storing EndRun logbook information\n");

    if (conn != NULL) {
      // close connection
      delete conn ;
    }

    return tr;
  }
private:
  OfflineClient* _offlineclient;
};

//
// OfflineNullEndRun
//
class OfflineNullEndRun : public Action {
public:
  OfflineNullEndRun() {}
  InDatagram* fire(InDatagram* dg) {
    return dg;
  }
  Transition* fire(Transition* tr) {
    return tr;
  }
};

//
// OfflineClient
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

            // get experiment list
            std::vector<LogBook::ExperDescr> experiments;
            std::string instrument = _instrument_name;

            conn->getExperiments(experiments, instrument);

//          std::cout << "Experiment list:" << std::endl;
//          for (size_t ii = 0 ; ii < experiments.size(); ii++) {
//              std::cout << " . " << experiments[ii].name;
//              std::cout << "  #" << experiments[ii].id << std::endl;
//          }
            for (size_t ii = 0 ; ii < experiments.size(); ii++) {
                if (experiments[ii].name.compare(_experiment_name) == 0) {
                    _experiment_number = (unsigned int) experiments[ii].id;
                    found = true;
                    break;
                }
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

    if (!found) {
      _experiment_number = 0;
      printf ("Error: OfflineClient(): experiment %s/%s not found, using experiment ID 0\n",
              _instrument_name, _experiment_name);
    }

  if (path && (strcmp(path, "/dev/null") != 0)) {
    // connect callbacks for real database
    callback(TransitionId::BeginRun, new OfflineDbBeginRun(this));
    callback(TransitionId::EndRun, new OfflineDbEndRun(this));
  } else {
    // connect callbacks for null database
    callback(TransitionId::BeginRun, new OfflineNullBeginRun());
    callback(TransitionId::EndRun, new OfflineNullEndRun);
  }
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
      printf("Completed allocating run number %d\n",_run_number);

      if (conn != NULL) {
        // close connection
        delete conn ;
      }
    }
  }
  return (returnVal);
}

//
// GetRunNumber
//
// This routine returns the run number previously allocated by AllocateRunNumber().
//
unsigned int OfflineClient::GetRunNumber() {
    return (_run_number);
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
