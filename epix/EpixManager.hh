/*
 * EpixManager.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXMANAGER_HH
#define EPIXMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
  class EpixServer;
  class EpixManager;
  class CfgClientNfs;
  class EpixConfigCache;
}

class Pds::EpixManager {
  public:
    EpixManager( EpixServer* server, unsigned d);
    Appliance& appliance( void ) { return _fsm; }

  private:
    Fsm& _fsm;
    EpixConfigCache& _cfg;
};

#endif
