/*
 * Epix100aManager.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIX100AMANAGER_HH
#define EPIX100AMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
  class Epix100aServer;
  class Epix100aManager;
  class CfgClientNfs;
  class Epix100aConfigCache;
}

class Pds::Epix100aManager {
  public:
    Epix100aManager( Epix100aServer* server, unsigned d);
    Appliance& appliance( void ) { return _fsm; }

  private:
    Fsm& _fsm;
    Epix100aConfigCache& _cfg;
};

#endif
