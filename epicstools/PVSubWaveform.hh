#ifndef Pds_PVSubWaveform_hh
#define Pds_PVSubWaveform_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds_Epics {
  class PVSubWaveform : public EpicsCA {
  public:
    PVSubWaveform(const char* pvName, PVMonitorCb* cb) : EpicsCA(pvName,0) {
      _monitor = cb;
    }
    ~PVSubWaveform() {}
  public:
    int   get_nelements() const { return _channel.nelements(); }
    void  set_nelements(int nelements) { _channel.set_nelements(nelements); }
    void  start_monitor() { _channel.start_monitor(); }
    void  stop_monitor() { _channel.stop_monitor(); }
  };
};

#endif
