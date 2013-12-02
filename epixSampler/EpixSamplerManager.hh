/*
 * EpixSamplerManager.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXSAMPLERMANAGER_HH
#define EPIXSAMPLERMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
    class EpixSamplerServer;
    class EpixSamplerManager;
    class CfgClientNfs;
    class EpixSamplerConfigCache;
  }

  class Pds::EpixSamplerManager {
    public:
      EpixSamplerManager( EpixSamplerServer* server, unsigned d);
      Appliance& appliance( void ) { return _fsm; }

    private:
      Fsm& _fsm;
      EpixSamplerConfigCache& _cfg;
  };

#endif
