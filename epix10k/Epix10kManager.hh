/*
 * Epix10kManager.hh
 *
 *  Created on: 2014.4.15
 *      Author: jackp
 */

#ifndef EPIX10KMANAGER_HH
#define EPIX10KMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
  class Epix10kServer;
  class Epix10kManager;
  class CfgClientNfs;
  class Epix10kConfigCache;
  class Epix10kL1Action;
}

class Pds::Epix10kManager {
  public:
    Epix10kManager( Epix10kServer* server, unsigned d);
    Appliance& appliance( void ) { return _fsm; }
    Pds::Epix10kL1Action * l1Action;

  private:
    Fsm& _fsm;
    Epix10kConfigCache& _cfg;
};

#endif
