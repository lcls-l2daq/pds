/*
 * EpixSManager.cc
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
#include "pds/epixS/EpixSServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
//#include "pdsdata/psddl/epix.ddl.h"
#include "pds/epixS/EpixSManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;


  class EpixSConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      EpixSConfigCache(const Src& src) :
        CfgCache(src, _epixSConfigType, StaticALlocationNumberOfConfigurationsForScanning * __size()) {}
    public:
     void printCurrent() {
       EpixSConfigType* cfg = (EpixSConfigType*)current();
       printf("EpixSConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return ((EpixSConfigType*)tc)->_sizeof(); }
      static int __size() {
        EpixSConfigType* foo = new EpixSConfigType(2,2,0x160, 0x180, 2);
        int size = (int) foo->_sizeof();
        delete foo;
        return size;
      }
 };
}


using namespace Pds;

class EpixSAllocAction : public Action {

  public:
   EpixSAllocAction(EpixSConfigCache& cfg, EpixSServer* svr)
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
   EpixSConfigCache& _cfg;
   EpixSServer*      _svr;
};

class EpixSUnmapAction : public Action {
  public:
    EpixSUnmapAction(EpixSServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("EpixSUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    EpixSServer* server;
};

class EpixSL1Action : public Action {
 public:
   EpixSL1Action(EpixSServer* svr);

   InDatagram* fire(InDatagram* in);

   EpixSServer* server;
   unsigned _lastMatchedFiducial;
//   unsigned _ioIndex;
   bool     _fiducialError;

};

EpixSL1Action::EpixSL1Action(EpixSServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* EpixSL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("EpixSL1Action::fire!\n");
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
        printf("EpixSL1Action::fire inner xtc not Id_EpixElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("EpixSL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("EpixSL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class EpixSConfigAction : public Action {

  public:
    EpixSConfigAction( Pds::EpixSConfigCache& cfg, EpixSServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~EpixSConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("EpixSConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        if ((_result = _server->configure( (EpixSConfigType*)_cfg.current()))) {
          printf("\nEpixSConfigAction::fire(tr) failed configuration\n");
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else {
        _server->offset(0);
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->samplerConfig()) {
        printf("EpixSConfigAction sending sampler config\n");
        in->insert(_server->xtcConfig(), _server->samplerConfig());
      } else {
        printf("EpixSConfigAction not sending sampler config\n");
      }
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** EpixSConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixSConfigCache&  _cfg;
    EpixSServer*      _server;
  unsigned           _result;
};

class EpixSBeginCalibCycleAction : public Action {
  public:
    EpixSBeginCalibCycleAction(EpixSServer* s, EpixSConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixSBeginCalibCycleAction:;fire(tr) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and ");
          _server->offset(_server->offset()+_server->myCount()+1);
          printf(" offset %u count %u\n", _server->offset(), _server->myCount());
          if ((_result = _server->configure( (EpixSConfigType*)_cfg.current()))) {
            printf("\nEpixSBeginCalibCycleAction::fire(tr) failed config\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("EpixSBeginCalibCycleAction:;fire(tr) enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->samplerConfig()) {
          printf("EpixSBeginCalibCycleAction sending sampler config\n");
          in->insert(_server->xtcConfig(), _server->samplerConfig());
        } else {
          printf("EpixSBeginCalibCycleAction not sending sampler config\n");
        }
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** EpixSConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    EpixSServer*       _server;
    EpixSConfigCache&  _cfg;
    unsigned          _result;
};


class EpixSUnconfigAction : public Action {
 public:
   EpixSUnconfigAction( EpixSServer* server, EpixSConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
   ~EpixSUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("EpixSUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("EpixSUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epixS Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   EpixSServer*      _server;
   EpixSConfigCache& _cfg;
   unsigned         _result;
};

class EpixSEndCalibCycleAction : public Action {
  public:
    EpixSEndCalibCycleAction(EpixSServer* s, EpixSConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixSEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** EpixSEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixSServer*      _server;
    EpixSConfigCache& _cfg;
    unsigned          _result;
};

EpixSManager::EpixSManager( EpixSServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new EpixSConfigCache(server->client())) {

   printf("EpixSManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int epixS = open( devName,  O_RDWR | O_NONBLOCK);
   printf("pgpcard %s file number %d\n", devName, epixS);
   if (epixS < 0) {
     sprintf(err, "EpixSManager::EpixSManager() opening %s failed", devName);
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
     printf("EpixSManager::EpixSManager() illegal port mask!! 0x%x\n", ports);
   }

   server->manager(this);
   server->setEpixS( epixS );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new EpixSAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new EpixSUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new EpixSConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new EpixSEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new EpixSDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new EpixSBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new EpixSEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new EpixSL1Action( server ) );
   _fsm.callback( TransitionId::Unconfigure, new EpixSUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new EpixSBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new EpixSEndRunAction( server ) );
}
