/*
 * EpixSManager.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIXSMANAGER_HH
#define EPIXSMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
  class EpixSServer;
  class EpixSManager;
  class CfgClientNfs;
  class EpixSConfigCache;
}

class Pds::EpixSManager {
  public:
    EpixSManager( EpixSServer* server, unsigned d);
    Appliance& appliance( void ) { return _fsm; }

  private:
    Fsm& _fsm;
    EpixSConfigCache& _cfg;
};

#endif
