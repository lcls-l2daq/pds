
#ifndef OFFLINECLIENT_HH
#define OFFLINECLIENT_HH

#include <vector>
#include "LogBook/Connection.h"

namespace Pds {

  class OfflineClient {
  public:
    OfflineClient(const char *path, const char *instrument, const char *experiment);
    OfflineClient(const char *path, const char *instrument);
    OfflineClient(const char *path, const char *instrument, unsigned station, bool verbose=true);
    int AllocateRunNumber(unsigned int *runNumber);
    int BeginNewRun(int runNumber);
    int reportOpenFile (int expt, int run, int stream, int chunk, std::string& host, std::string& dirpath, bool ffb=false);
    int reportDetectors (int expt, int run, std::vector<std::string>& names);
    unsigned int GetExperimentNumber();
    const char * GetExperimentName();
    const char * GetInstrumentName();
    const char * GetPath();
    unsigned int GetStationNumber();
  private:
    const char * _path;
    const char * _instrument_name;
    const char * _experiment_name;
    unsigned int _run_number;
    unsigned int _experiment_number;
    // Caution!  _experiment_descr is filled from offline database
    // Contents may not be the same as _experiment_name, _experiment_number
    LogBook::ExperDescr _experiment_descr;
    unsigned int _station_number;
    bool _verbose;
  };

}

#endif
