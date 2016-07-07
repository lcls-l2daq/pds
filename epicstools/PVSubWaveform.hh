#ifndef Pds_PVSubWaveform_hh
#define Pds_PVSubWaveform_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds_Epics {
  class PVSubWaveform : public EpicsCA {
  public:
    PVSubWaveform(const char* pvName, PVMonitorCb* cb, const int maxElements) :
      EpicsCA(pvName,cb,maxElements) {}
    ~PVSubWaveform() {}
  public:
    int   get_nelements() const { return _channel.nelements(); }
    dbr_short_t status() { return _status; }
    dbr_short_t severity() { return _severity; }
  };
};

#endif
