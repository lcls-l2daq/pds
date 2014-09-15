/*
 * Manager.hh
 *
 */

#ifndef GenericPgpManager_hh
#define GenericPgpManager_hh

#include "pds/client/Fsm.hh"

namespace Pds {
  namespace GenericPgp {
    class Server;
    class ConfigCache;

    class Manager {
    public:
      Manager( Server* server, unsigned d);
      Appliance& appliance( void ) { return _fsm; }
      
    private:
      Fsm& _fsm;
      ConfigCache& _cfg;
    };
  };
};

#endif
