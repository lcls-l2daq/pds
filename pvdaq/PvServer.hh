#ifndef Pds_PvDaq_PvServer_hh
#define Pds_PvDaq_PvServer_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace PvDaq {
    class PvServer : public Pds_Epics::EpicsCA,
                     public Pds_Epics::PVMonitorCb {
    public:
      PvServer(const char*);
      ~PvServer();
    public:
      void connected(bool);
      void updated();
    public:
      int  fetch      (char* copyTo, size_t wordSize);
      int  fetch_u32  (char* copyTo, size_t nwords, size_t offset=0);
      void update     ();
    };
  };
};

#endif
