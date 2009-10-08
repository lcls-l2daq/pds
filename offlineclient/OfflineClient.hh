
#ifndef OFFLINECLIENT_HH
#define OFFLINECLIENT_HH

#include "pds/client/Fsm.hh"

namespace Pds {

  class OfflineClient : public Fsm {
  public:
    OfflineClient(const char *path, const char *instrument, const char *experiment);
    int AllocateRunNumber(unsigned int *runNumber);
    unsigned int GetRunNumber();
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
  };

}

#endif
