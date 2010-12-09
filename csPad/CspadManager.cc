/*
 * CspadManager.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/csPad/CspadServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "CspadManager.hh"
#include "CspadServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/config/CfgClientNfs.hh"

using namespace Pds;

class CspadAllocAction : public Action {

  public:
   CspadAllocAction(CfgClientNfs& cfg)
      : _cfg(cfg) {}

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.initialize(alloc.allocation());
      return tr;
   }

 private:
   CfgClientNfs& _cfg;
};

class CspadL1Action : public Action {
 public:
   CspadL1Action(CspadServer* svr);

   InDatagram* fire(InDatagram* in);

   CspadServer* server;

};

CspadL1Action::CspadL1Action(CspadServer* svr) : server(svr) {}

InDatagram* CspadL1Action::fire(InDatagram* in) {
  if (in->datagram().xtc.damage.value() == 0) {
    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    unsigned error = 0;
    unsigned evrFiducials = dg.seq.stamp().fiducials();
    char*    payload = dg.xtc.payload();

    for (unsigned i=0; i<server->numberOfQuads(); i++) {
      data = (Pds::Pgp::DataImportFrame*) ( payload + sizeof(Xtc) + (i * server->payloadSize()) );
      if (evrFiducials != data->fiducials()) {
        error |= 1<<i;
        printf("CspadL1Action setting user damage due to fiducial mismatch evr(%u) cspad(%u) in quads(0x%x)\n",
            evrFiducials, data->fiducials(), error);
      }
    }
    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
    }
  }
  return in;
}

class CspadConfigAction : public Action {

 public:
   CspadConfigAction( const Src& src0,
                        CfgClientNfs* cfg,
                        CspadServer* server)
      : _cfgtc( _CsPadConfigType, src0 ),
        _cfg( cfg ),
        _server( server )
   {}

   ~CspadConfigAction() {}

   InDatagram* fire(InDatagram* dg) {
      // insert assumes we have enough space in the input datagram
      dg->insert(_cfgtc, &_config);
      if( _result ) {
         printf( "*** CspadConfigAction found configuration errors _result(0x%x)\n", _result );
         if (dg->datagram().xtc.damage.value() == 0) {
           dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
           dg->datagram().xtc.damage.userBits(_result);
         }
      }
      return dg;
   }

   Transition* fire(Transition* tr) {
      _result = 0;
      int ret = _cfg->fetch( *tr, _CsPadConfigType, &_config, sizeof(_config) );
      if (ret != sizeof(_config)) {
         printf("CspadConfigAction: failed to retrieve configuration: ");
         if (ret < 0) printf("(%d) %s.\n", errno, strerror(errno) );
         else printf("length returned %u, config size %u", ret, sizeof(_config));
         _result = 0xcf;
      } else {
         _result = _server->configure( _config );
      }

      return tr;
   }

 private:
   CsPadConfigType _config;
   Xtc             _cfgtc;
   Src             _src;
   CfgClientNfs*   _cfg;
   CspadServer*    _server;
   unsigned       _result;
};


class CspadUnconfigAction : public Action {
 public:
   CspadUnconfigAction( CspadServer* server ) : _server( server ) {}
   ~CspadUnconfigAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _result ) {
         printf( "*** Found %d cspad Unconfig errors\n", _result );
         dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         dg->datagram().xtc.damage.userBits(0xda);
      }
      return dg;
   }

   Transition* fire(Transition* tr) {
      _result = 0;
      _result += _server->unconfigure();
      return tr;
   }

 private:
   CspadServer* _server;
   unsigned _result;
};


CspadManager::CspadManager( CspadServer* server, CfgClientNfs* cfg ) : _fsm(*new Fsm) {

   printf("CspadManager being initialized...\n" );

   int cspad = open( "/dev/pgpcard",  O_RDWR);
   // What to do if the open fails?
   server->setCspad( cspad );

   const Src& src0 = server->client();

   _fsm.callback( TransitionId::Configure, new CspadConfigAction( src0, cfg, server ) );
   _fsm.callback( TransitionId::Unconfigure, new CspadUnconfigAction( server ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new CspadBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new CspadEndRunAction( server ) );
   _fsm.callback( TransitionId::Map, new CspadAllocAction( *cfg ) );
   _fsm.callback( TransitionId::L1Accept, new CspadL1Action( server ) );
}
