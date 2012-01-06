#ifndef PHASICSMANAGER_HH
#define PHASICSMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class PhasicsServer;
    class PhasicsManager;
    class CfgClientNfs;
    class PhasicsConfigCache;
  }

  class Pds::PhasicsManager {
    public:
      PhasicsManager( PhasicsServer* server);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      PhasicsConfigCache& _cfg;
  };

#endif
