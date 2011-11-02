
#ifndef OFFLINECLIENT_HH
#define OFFLINECLIENT_HH

#include "LogBook/Connection.h"

namespace Pds {

  class OfflineClient {
  public:
    OfflineClient(const char *path, const char *instrument, const char *experiment);
    OfflineClient(const char *path, const char *instrument);
    int AllocateRunNumber(unsigned int *runNumber);
    int reportOpenFile (int expt, int run, int stream, int chunk, std::string& host, std::string& dirpath);
    unsigned int GetExperimentNumber();
    const char * GetExperimentName();
    const char * GetInstrumentName();
    const char * GetPath();
  private:
    const char * _path;
    const char * _instrument_name;
    const char * _experiment_name;
    unsigned int _run_number;
    unsigned int _experiment_number;
    // Caution!  _experiment_descr is filled from offline database
    // Contents may not be the same as _experiment_name, _experiment_number
    LogBook::ExperDescr _experiment_descr;
  };

}

#endif
