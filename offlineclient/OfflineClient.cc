#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "OfflineClient.hh"

#include "LogBook/Connection.h"

using namespace Pds;

//
// Partition Descriptor
//
PartitionDescriptor::PartitionDescriptor(const char *partition) :
  _partition_name(""),
  _instrument_name(""),
  _station_number(0u),
  _valid(false)
{
  unsigned int station = 0u;

  if (partition) {
    char instr[64];
    char *pColon;
    strncpy(instr, partition, sizeof(instr));
    pColon = index(instr, ':');
    if (pColon) {
      *pColon++ = '\0';
      if (isdigit(*pColon)) {
        errno = 0;
        station = strtoul(pColon, (char **)NULL, 10);
        if (!errno) {
          _valid = true;
        }
      }
    } else {
      _valid = true;
    }
    if (_valid) {
      _partition_name = partition;
      _instrument_name = instr;
      _station_number = station;
    }
  }
}

std::string PartitionDescriptor::GetPartitionName()
{
  return (_partition_name);
}

std::string PartitionDescriptor::GetInstrumentName()
{
  return (_instrument_name);
}

unsigned int PartitionDescriptor::GetStationNumber()
{
  return (_station_number);
}

bool PartitionDescriptor::valid()
{
  return (_valid);
}

//
// OfflineClient (current experiment name passed in)
//
OfflineClient::OfflineClient(const char* path, const char* instrument_name, const char* experiment_name) :
    _path (path),
    _instrument_name (instrument_name),
    _experiment_name (experiment_name),
    _station_number(0),
    _verbose(true)
{
    fprintf(stderr, "*** WARNING! _station_number hardcoded to 0 in %s\n", __PRETTY_FUNCTION__); // FIXME
  
    bool found = false;
    printf("entered OfflineClient(path=%s, instr=%s, exp=%s)\n", _path, _instrument_name, _experiment_name);

    // translate experiment name to experiment number
    LogBook::Connection * conn = NULL;

    try {
        _experiment_number = 0;
        if (strcmp(path, "/dev/null") == 0) {
            printf("fake it (path=/dev/null)\n");
            _experiment_number = 1;
            found = true;
        } else {
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
      printf ("OfflineClient(): experiment %s:%u/%s not found\n",
              _instrument_name, _station_number, _experiment_name);
    }

    printf ("OfflineClient(): experiment %s:%u/%s (#%d) \n",
            _instrument_name, _station_number, _experiment_name, _experiment_number);
}

//
// OfflineClient (current experiment name retrieved from database)
//
OfflineClient::OfflineClient(const char* path, PartitionDescriptor pd, bool verbose) :
    _path (path),
    _instrument_name (pd.GetInstrumentName().c_str()),
    _station_number(pd.GetStationNumber()),
    _verbose(verbose)
{
    if (_verbose) {
      printf("entered OfflineClient(path=%s, instr=%s, station=%u)\n", _path, _instrument_name, _station_number);
    }
    bool success = false;
    _experiment_name = OFFLINECLIENT_DEFAULT_EXPNAME;
    _experiment_number = OFFLINECLIENT_DEFAULT_EXPNUM;

    if (pd.valid()) {
      LogBook::Connection * conn = NULL;
      try {
        conn = LogBook::Connection::open(path);
        if (conn == NULL) {
          fprintf(stderr, "LogBook::Connection::connect() failed\n");
        } else {
          // begin transaction
          conn->beginTransaction();

          // get current experiment
          std::string instrument = _instrument_name;
          
          if (conn->getCurrentExperiment(_experiment_descr, instrument, _station_number)) {
            _experiment_name = _experiment_descr.name.c_str();
            _experiment_number = _experiment_descr.id;
            success = true;
          } else {
            fprintf (stderr, "%s: No experiment found for instrument %s:%u\n",
                     __FUNCTION__, _instrument_name, _station_number);
          }
        }
      } catch (const LogBook::ValueTypeMismatch& e) {
        fprintf (stderr, "Parameter type mismatch %s:\n", e.what());

      } catch (const LogBook::WrongParams& e) {
        fprintf (stderr, "Problem with parameters %s:\n", e.what());
      
      } catch (const LogBook::DatabaseError& e) {
        fprintf (stderr, "Database operation failed: %s\n", e.what());
      }

      if (conn != NULL) {
          // close connection
          delete conn ;
      }
      if (_verbose) {
        printf ("%s: instrument %s:%u experiment %s (#%d) \n", __FUNCTION__,
                _instrument_name, _station_number, _experiment_name, _experiment_number);
      }
    } else {
      printf("%s: partition descriptor not valid\n", __PRETTY_FUNCTION__);
    }
}

//
// AllocateRunNumber
//
// Allocate a new run number from the database.
// If database is NULL then the run number is set to 0.
//
// NOTE: Allocating a run number won't result in any runs automatically
// registered in LogBook or any other database.
//
// SEE ALSO: BeginNewRun()
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
      printf("Allocating run number (instrument '%s:%u', experiment '%s')\n", _instrument_name, _station_number, _experiment_name);
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

//
// BeginNewRun
//
// Begin new run: create a new run entry in the database.
//
// NOTE: Allocating a run number won't result in any runs automatically
// registered in LogBook or any other database.
//
// SEE ALSO: AllocateRunNumber()
//
// RETURNS: 0 if successful, otherwise -1
//
int OfflineClient::BeginNewRun(int runNumber) {
  LusiTime::Time now;
  LogBook::Connection * conn = NULL;
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if (runNumber && _instrument_name && _experiment_name) {

    // in case of NULL database, do nothing
    if ((_path == (char *)NULL) || (strcmp(_path, "/dev/null") == 0)) {
      returnVal = 0;  // OK
    } else {
      printf("Creating new run number %d database entry\n", runNumber);
      try {
        conn = LogBook::Connection::open(_path);

        if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          // LogBook: begin run
          now = LusiTime::Time::now();
          conn->beginRun(_instrument_name,
                         _experiment_name,
                         runNumber, "DATA", now); // DATA/CALIB

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
        printf("Completed creating new run rumber %d database entry\n", runNumber);
      }

      if (conn != NULL) {
        // close connection
        delete conn ;
      }
    }
  }

  if (-1 == returnVal) {
    printf("%s returning error\n", __FUNCTION__);
  }

  return (returnVal);
}

int OfflineClient::reportOpenFile (int expt, int run, int stream, int chunk, std::string& host, std::string& dirpath, bool ffb) {
  LogBook::Connection * conn = NULL;
  int returnVal = -1;  // default return is ERROR

  // sanity check
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
          // fast feedback option
          if (ffb) {
            conn->reportOpenFile(expt, run, stream, chunk, host, dirpath, "ffb"); // fast feedback
          } else {
            conn->reportOpenFile(expt, run, stream, chunk, host, dirpath);
          }
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
    fprintf(stderr, "Error reporting open file expt %d run %d stream %d chunk %d\n",
            expt, run, stream, chunk);
  }

  return (returnVal);
}

int OfflineClient::reportDetectors (int expt, int run, std::vector<std::string>& names) {
  LogBook::Connection * conn = NULL;
  int returnVal = -1;  // default return is ERROR

  // sanity check
  if ((names.size() > 0) && run && _instrument_name && _experiment_name) {
    // in case of NULL database, report nothing
    if ((_path == (char *)NULL) || (strcmp(_path, "/dev/null") == 0)) {
      returnVal = 0;  // OK
    } else {
      try {
        conn = LogBook::Connection::open(_path);

        if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          std::vector<std::string>::iterator nn;
          for (nn = names.begin(); nn != names.end(); nn++) {
            conn->createRunAttr(_instrument_name, _experiment_name, run,
                                "DAQ_Detectors", *nn, "", "");
          }
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

//
// GetStationNumber
//
unsigned int OfflineClient::GetStationNumber() {
    return (_station_number);
}
