#ifndef ENCODERMANAGER_HH
#define ENCODERMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class CspadServer;
    class CspadManager;
    class CfgClientNfs;
    class CspadConfigCache;
  }

  class Pds::CspadManager {
    public:
      CspadManager( CspadServer* server);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      CspadConfigCache& _cfg;
  };

#endif
