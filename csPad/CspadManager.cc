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
#include "pds/config/CsPadDataType.hh"
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
//  printf("CspadL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_CspadElement) {
        payload = xtc->payload();
      } else {
        printf("CspadLiAction::fire inner xtc not Id_CspadElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("CspadLiAction::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    for (unsigned i=0; i<server->numberOfQuads(); i++) {
      data = (Pds::Pgp::DataImportFrame*) ( payload + (i * server->payloadSize()) );
      if (evrFiducials != data->fiducials()) {
        error |= 1<<i;
        printf("CspadL1Action::fire(in) fiducial mismatch evr(%u) cspad(%u) in quad %u\n",
            evrFiducials, data->fiducials(), i);
      }
    }
    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("CspadL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
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
     printf("CspadConfigAction::fire(InDatagram)\n");
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
      printf("CspadConfigAction::fire(Transition)\n");
      int ret = _cfg->fetch( *tr, _CsPadConfigType, &_config, sizeof(_config) );
      if (ret != sizeof(_config)) {
         printf("CspadConfigAction: failed to retrieve configuration: ");
         if (ret < 0) printf("(%d) %s.\n", errno, strerror(errno) );
         else printf("length returned %u, config size %u", ret, sizeof(_config));
         _result = 0xcf;
      } else {
         _result = _server->configure( _config );
         _server->enable();
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

//class CspadBeginCalibAction : public Action {
//
//};


class CspadUnconfigAction : public Action {
 public:
   CspadUnconfigAction( CspadServer* server ) : _server( server ) {}
   ~CspadUnconfigAction() {}

   InDatagram* fire(InDatagram* dg) {
     printf("CspadUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d cspad Unconfig errors\n", _result );
         dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         dg->datagram().xtc.damage.userBits(0xda);
      }
      ::usleep(250000);
      return dg;
   }

   Transition* fire(Transition* tr) {
     printf("CspadUnconfigAction:;fire(Transition)\n");
      _result = 0;
      _server->disable();
      _result += _server->unconfigure();
      return tr;
   }

 private:
   CspadServer* _server;
   unsigned _result;
};

class CspadEnableAction : public Action {
  public:
    CspadEnableAction( CspadServer* server ) : _server( server ) {}
    ~CspadEnableAction() {}

    Transition* fire(Transition* tr) {
      printf("CspadEnableAction:;fire(Transition)\n");
//      _server->enable();
      return tr;
    }

  private:
    CspadServer* _server;
};

class CspadDisableAction : public Action {
  public:
    CspadDisableAction( CspadServer* server ) : _server( server ) {}
    ~CspadDisableAction() {}

    Transition* fire(Transition* tr) {
      printf("CspadDisableAction:;fire(Transition)\n");
      return tr;
    }

  private:
    CspadServer* _server;
};


CspadManager::CspadManager( CspadServer* server, CfgClientNfs* cfg ) : _fsm(*new Fsm) {

   printf("CspadManager being initialized... " );

   int cspad = open( "/dev/pgpcard",  O_RDWR);
   printf("pgpcard file number %d\n", cspad);
   if (cspad < 0) {
     perror("CspadManager::CspadManager() opening pgpcard failed: ");
     // What else to do if the open fails?
     ::exit(-1);
   }

   server->setCspad( cspad );

   const Src& src0 = server->client();

   _fsm.callback( TransitionId::Map, new CspadAllocAction( *cfg ) );
   _fsm.callback( TransitionId::Configure, new CspadConfigAction( src0, cfg, server ) );
   _fsm.callback( TransitionId::Enable, new CspadEnableAction( server ) );
//   _fsm.callback( TransitionId::BeginCalibCycle, new CspadBeginCalibCycleAction( server ) );
   _fsm.callback( TransitionId::L1Accept, new CspadL1Action( server ) );
//   _fsm.callback( TransitionId::EndCalibCycle, new CspadEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new CspadUnconfigAction( server ) );
   _fsm.callback( TransitionId::Disable, new CspadDisableAction( server ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new CspadBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new CspadEndRunAction( server ) );
}
