#ifndef ENCODERMANAGER_HH
#define ENCODERMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class CspadServer;
    class CspadManager;
    class CfgClientNfs;
  }

  class Pds::CspadManager {
    public:
      CspadManager( CspadServer* server,
          CfgClientNfs* cfg );
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
  };

#endif
