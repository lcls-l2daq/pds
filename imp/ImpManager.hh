#ifndef IMPMANAGER_HH
#define IMPMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class ImpServer;
    class ImpManager;
    class CfgClientNfs;
    class ImpConfigCache;
  }

  class Pds::ImpManager {
    public:
      ImpManager( ImpServer* server, int d);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      ImpConfigCache& _cfg;
  };

#endif
