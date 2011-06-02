#ifndef XAMPSMANAGER_HH
#define XAMPSMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class XampsServer;
    class XampsManager;
    class CfgClientNfs;
    class XampsConfigCache;
  }

  class Pds::XampsManager {
    public:
      XampsManager( XampsServer* server);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      XampsConfigCache& _cfg;
  };

#endif
