#ifndef FEXAMPMANAGER_HH
#define FEXAMPMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class FexampServer;
    class FexampManager;
    class CfgClientNfs;
    class FexampConfigCache;
  }

  class Pds::FexampManager {
    public:
      FexampManager( FexampServer* server);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      FexampConfigCache& _cfg;
  };

#endif
