#ifndef Pds_PvDaq_BeamMonitorServer_hh
#define Pds_PvDaq_BeamMonitorServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <vector>

namespace Pds {
  namespace PvDaq {
    class PvServer;
    class BeamMonitorServer : public Pds::PvDaq::Server, 
                              public Pds_Epics::PVMonitorCb {
    public:
      BeamMonitorServer(const char*, const DetInfo&);
      ~BeamMonitorServer();
    public:
      // Server interface
      int         fill( char*, const void* );
      Transition* fire(Transition*);
      InDatagram* fire(InDatagram*);
      // PvMonitorCb
      void        updated();
    private:
      enum { NCHANNELS=16 };
      Pds_Epics::EpicsCA*    _raw;
      std::vector<PvServer*> _config_pvs;
      PvServer*              _chan_enable;
      bool                   _enabled;
      unsigned               _chan_mask;
      unsigned               _rawlen[NCHANNELS];
      unsigned               _len_125;
      unsigned               _len_5;
    };
  };
};

#endif
