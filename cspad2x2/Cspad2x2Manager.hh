#ifndef CSPAD2x2MANAGER_HH
#define CSPAD2x2MANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class Cspad2x2Server;
    class Cspad2x2Manager;
    class CfgClientNfs;
    class Cspad2x2ConfigCache;
  }

  class Pds::Cspad2x2Manager {
    public:
      Cspad2x2Manager( Cspad2x2Server* server, unsigned d);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm&              _fsm;
      Cspad2x2ConfigCache& _cfg;
  };

#endif
