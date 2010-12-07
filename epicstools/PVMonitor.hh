#ifndef Pds_PVMonitor_hh
#define Pds_PVMonitor_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds_Epics {
  class PVMonitor : public EpicsCA {
  public:
    PVMonitor(const char* pvName, PVMonitorCb& cb) : EpicsCA(pvName,&cb) {}
    ~PVMonitor() {}
  };

};

#endif
