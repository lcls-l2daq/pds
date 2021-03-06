
#ifndef OFFLINECLIENT_HH
#define OFFLINECLIENT_HH

#include <vector>
#include "LogBook/Connection.h"

#define OFFLINECLIENT_DEFAULT_EXPNAME ((char *)NULL)
#define OFFLINECLIENT_DEFAULT_EXPNUM  0
#define OFFLINECLIENT_DEFAULT_STATION 0u
#define OFFLINECLIENT_MAX_PARMS       1000

namespace Pds {

  class PartitionDescriptor {
  public:
    PartitionDescriptor(const char *name);
    std::string GetPartitionName();
    std::string GetInstrumentName();
    unsigned int GetStationNumber();
    bool valid();

  private:
    std::string _partition_name;
    std::string _instrument_name;
    unsigned int _station_number;
    bool _valid;
  };

  class OfflineClient {
  public:
    OfflineClient(const char* path, PartitionDescriptor& pd, const char *experiment_name, bool verbose=true);
    OfflineClient(const char* path, PartitionDescriptor& pd, bool verbose=true);
    int AllocateRunNumber(unsigned int *runNumber);
    int BeginNewRun(int runNumber);
    int reportOpenFile (int expt, int run, int stream, int chunk, std::string& host, std::string& dirpath, bool ffb=false);
    int reportDetectors (int expt, int run, std::vector<std::string>& names);
    int reportTotals (int expt, int run, long events, long damaged, double gigabytes);

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
    PartitionDescriptor _pd;
  };

}

#endif
