/*
 * pnCCDManager.hh
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#ifndef PNCCDMANAGER_HH
#define PNCCDMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

#include <string>

namespace Pds {
    class pnCCDServer;
    class pnCCDManager;
    class CfgClientNfs;
    class pnCCDConfigCache;
  }

  class Pds::pnCCDManager {
    public:
      pnCCDManager( pnCCDServer* server, unsigned d, std::string& sConfigFile);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm&              _fsm;
      pnCCDConfigCache& _cfg;
      GenericPool*      _occPool;
      std::string       _sConfigFile;
  };

#endif
