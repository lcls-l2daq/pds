#ifndef Pds_LeCroy_ConfigServer_hh
#define Pds_LeCroy_ConfigServer_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace LeCroy {
    class Server;
    class ConfigServer : public Pds_Epics::EpicsCA,
                         public Pds_Epics::PVMonitorCb {
    public:
      ConfigServer(const char*, Server*);
      ConfigServer(const char*, Server*, bool);
      ~ConfigServer();
    public:
      void updated();
    public:
      int  fetch      (char* copyTo);
      void update     ();
    private:
      Server* _srv;
      bool    _trig;
    };
  }
}

#endif
