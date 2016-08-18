#ifndef Pds_PvDaq_BeamMonitorServer_hh
#define Pds_PvDaq_BeamMonitorServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <string>
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
      void        config_updated();
      enum { NCHANNELS=16 };
    private:
      std::string            _pvbase;
      Pds_Epics::EpicsCA*    _raw;
      PvServer*              _raw_nord;
      uint32_t               _exp_nord;
      Pds_Epics::EpicsCA*    _cfg_done;
      PvServer*              _cfg_errs;
      PvServer*              _sync;
      std::vector<PvServer*> _config_pvs;
      std::vector<PvServer*> _offset_pvs;
      PvServer*              _chan_enable;
      PvServer*              _ioc_version;
      PvServer*              _api_version;
      bool                   _enabled;
      bool                   _configured;
      unsigned               _chan_mask;
      uint32_t               _Length[NCHANNELS];
      int32_t                _Offset[NCHANNELS];
      char *                 _ConfigBuff;
      Pds_Epics::PVMonitorCb* _configMonitor;
      unsigned               _nprint;
      ClockTime              _last;
      unsigned               _rdp;
      unsigned               _wrp;
      std::vector<char*>     _pool;
    };
  };
};

#endif
