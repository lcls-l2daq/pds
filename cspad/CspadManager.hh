#ifndef CSPADMANAGER_HH
#define CSPADMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
#include "pds/cspad/CompressionProcessor.hh"

namespace Pds {
    class CspadServer;
    class CspadManager;
    class CfgClientNfs;
    class CspadConfigCache;
  }

  class Pds::CspadManager {
    public:
      CspadManager( CspadServer* server, unsigned d);
      Appliance& appliance( void ) { return _fsm; }
//      Appliance& appProcessor( void ) { return _appProcessor; }

    private:
      Fsm&              _fsm;
      CspadConfigCache& _cfg;
//      Appliance&        _appProcessor;
//      Pds::CspadCompressionProcessor _compressionProcessor;
  };

#endif
