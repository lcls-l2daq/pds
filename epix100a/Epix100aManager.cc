/*
 * Epix100aManager.cc
 *
 *  Created on: 2014.7.31
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
#include "pds/config/EpixConfigType.hh"
#include "pds/epix100a/Epix100aServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
//#include "pdsdata/psddl/epix.ddl.h"
#include "pds/epix100a/Epix100aManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;


  class Epix100aConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      Epix100aConfigCache(const Src& src) :
        CfgCache(src, _epix100aConfigType, StaticALlocationNumberOfConfigurationsForScanning * __size()) {}
    public:
     void printCurrent() {
       Epix100aConfigType* cfg = (Epix100aConfigType*)current();
       printf("Epix100aConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return ((Epix100aConfigType*)tc)->_sizeof(); }
      static int __size() {
        Epix100aConfigType* foo = new Epix100aConfigType(2,2,0x160, 0x180, 2);
        int size = (int) foo->_sizeof();
        delete foo;
        return size;
      }
 };
}


using namespace Pds;

class Epix100aAllocAction : public Action {

  public:
   Epix100aAllocAction(Epix100aConfigCache& cfg, Epix100aServer* svr)
      : _cfg(cfg), _svr(svr) {
   }

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.init(alloc.allocation());
      _svr->allocated();
      return tr;
   }

 private:
   Epix100aConfigCache& _cfg;
   Epix100aServer*      _svr;
};

class Epix100aUnmapAction : public Action {
  public:
    Epix100aUnmapAction(Epix100aServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("Epix100aUnmapAction::fire(tr)\n");
      return tr;
    }

  private:
    Epix100aServer* server;
};

class Epix100aL1Action : public Action {
 public:
   Epix100aL1Action(Epix100aServer* svr);

   InDatagram* fire(InDatagram* in);

   Epix100aServer* server;
   unsigned _lastMatchedFiducial;
//   unsigned _ioIndex;
   bool     _fiducialError;

};

Epix100aL1Action::Epix100aL1Action(Epix100aServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* Epix100aL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("Epix100aL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
        payload = xtc->payload();
      } else {
        printf("Epix100aL1Action::fire inner xtc not Id_EpixElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("Epix100aL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("Epix100aL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class Epix100aConfigAction : public Action {

  public:
    Epix100aConfigAction( Pds::Epix100aConfigCache& cfg, Epix100aServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~Epix100aConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("Epix100aConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        if ((_result = _server->configure( (Epix100aConfigType*)_cfg.current()))) {
          printf("\nEpix100aConfigAction::fire(tr) failed configuration\n");
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else {
        _server->offset(0);
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix100aConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->samplerConfig()) {
        printf("Epix100aConfigAction sending sampler config\n");
        in->insert(_server->xtcConfig(), _server->samplerConfig());
      } else {
        printf("Epix100aConfigAction not sending sampler config\n");
      }
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** Epix100aConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    Epix100aConfigCache&  _cfg;
    Epix100aServer*      _server;
  unsigned           _result;
};

class Epix100aBeginCalibCycleAction : public Action {
  public:
    Epix100aBeginCalibCycleAction(Epix100aServer* s, Epix100aConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("Epix100aBeginCalibCycleAction:;fire(tr) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and ");
          _server->offset(_server->offset()+_server->myCount()+1);
          printf(" offset %u count %u\n", _server->offset(), _server->myCount());
          if ((_result = _server->configure( (Epix100aConfigType*)_cfg.current()))) {
            printf("\nEpix100aBeginCalibCycleAction::fire(tr) failed config\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("Epix100aBeginCalibCycleAction:;fire(tr) enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix100aBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->samplerConfig()) {
          printf("Epix100aBeginCalibCycleAction sending sampler config\n");
          in->insert(_server->xtcConfig(), _server->samplerConfig());
        } else {
          printf("Epix100aBeginCalibCycleAction not sending sampler config\n");
        }
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** Epix100aConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    Epix100aServer*       _server;
    Epix100aConfigCache&  _cfg;
    unsigned          _result;
};


class Epix100aUnconfigAction : public Action {
 public:
   Epix100aUnconfigAction( Epix100aServer* server, Epix100aConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
   ~Epix100aUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("Epix100aUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("Epix100aUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epix100a Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   Epix100aServer*      _server;
   Epix100aConfigCache& _cfg;
   unsigned         _result;
};

class Epix100aEndCalibCycleAction : public Action {
  public:
    Epix100aEndCalibCycleAction(Epix100aServer* s, Epix100aConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("Epix100aEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();

      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix100aEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** Epix100aEndCalibCycleAction found unconfigure errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      _server->disable();
      return in;
    }

  private:
    Epix100aServer*      _server;
    Epix100aConfigCache& _cfg;
    unsigned          _result;
};

Epix100aManager::Epix100aManager( Epix100aServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new Epix100aConfigCache(server->client())) {

   printf("Epix100aManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int epix100a = open( devName,  O_RDWR | O_NONBLOCK);
   printf("pgpcard %s file number %d\n", devName, epix100a);
   if (epix100a < 0) {
     sprintf(err, "Epix100aManager::Epix100aManager() opening %s failed", devName);
     perror(err);
     // What else to do if the open fails?
     ::exit(-1);
   }

   unsigned offset = 0;
   while ((((ports>>offset) & 1) == 0) && (offset < 5)) {
     offset += 1;
   }

   Pgp::Pgp::portOffset(offset);

   if (offset >= 4) {
     printf("Epix100aManager::Epix100aManager() illegal port mask!! 0x%x\n", ports);
   }

   server->manager(this);
   server->setEpix100a( epix100a );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new Epix100aAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new Epix100aUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new Epix100aConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new Epix100aEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new Epix100aDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new Epix100aBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new Epix100aEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new Epix100aL1Action( server ) );
   _fsm.callback( TransitionId::Unconfigure, new Epix100aUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new Epix100aBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new Epix100aEndRunAction( server ) );
}
