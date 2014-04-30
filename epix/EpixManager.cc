/*
 * EpixManager.cc
 *
 *  Created on: 2013.11.9
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
#include "pds/epix/EpixServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
//#include "pdsdata/psddl/epix.ddl.h"
#include "pds/epix/EpixManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;

//  class MyCfgCache {
//  public:
//    MyCfgCache(const Src&    src,
//         const TypeId& id,
//         int           size) :
//           _config    (src),
//           _type      (id),
//           _configtc  (id, src) {
//      unsigned* u = (unsigned*) & _myconfig;
//      for (unsigned i=0; i<(sizeof(EpixConfigType)/sizeof(unsigned)); i++) {
//        u[i] = 0;
//      }
//    };
//
//    virtual ~MyCfgCache(){}
//  public:
//    bool        changed() const { return false; }
//    bool        scanning() const { return false; }
//    const void* current() const { return &_myconfig; }
//    void        record (InDatagram*d) const {}
//  public:
//    void        init   (const Allocation& alloc) { _config.initialize(alloc); }
//    int         fetch  (Transition* tr) {
//      int len = sizeof(EpixConfigType);
//      printf("MyCfgCache::fetch configuration type %x is size %u bytes\n", _type.value(), len);
//      _configtc.damage = 0;
//      _configtc.extent = sizeof(Xtc) + _size(&_myconfig);
//      return len;
//    }
//
//    void        next   () {}
//    Damage&     damage () {return _configtc.damage;}
//  private:
//    virtual int _size  (void*) const = 0;
//  private:
//    CfgClientNfs _config;
//    TypeId       _type;
//    Xtc          _configtc;
//    EpixConfigType _myconfig;
//  };
//

  class EpixConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      EpixConfigCache(const Src& src) :
        CfgCache(src, _epixConfigType, StaticALlocationNumberOfConfigurationsForScanning * 135616) {}
    public:
     void printCurrent() {
       EpixConfigType* cfg = (EpixConfigType*)current();
       printf("EpixConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return ((EpixConfigType*)tc)->_sizeof(); }
  };
}


using namespace Pds;

class EpixAllocAction : public Action {

  public:
   EpixAllocAction(EpixConfigCache& cfg, EpixServer* svr)
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
   EpixConfigCache& _cfg;
   EpixServer*      _svr;
};

class EpixUnmapAction : public Action {
  public:
    EpixUnmapAction(EpixServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("EpixUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    EpixServer* server;
};

class EpixL1Action : public Action {
 public:
   EpixL1Action(EpixServer* svr);

   InDatagram* fire(InDatagram* in);

   EpixServer* server;
   unsigned _lastMatchedFiducial;
//   unsigned _ioIndex;
   bool     _fiducialError;

};

EpixL1Action::EpixL1Action(EpixServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* EpixL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("EpixL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_EpixElement) {
        payload = xtc->payload();
      } else {
        printf("EpixL1Action::fire inner xtc not Id_EpixElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("EpixL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("EpixL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class EpixConfigAction : public Action {

  public:
    EpixConfigAction( Pds::EpixConfigCache& cfg, EpixServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~EpixConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("EpixConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        if ((_result = _server->configure( (EpixConfigType*)_cfg.current()))) {
          printf("\nEpixConfigAction::fire(tr) failed configuration\n");
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else {
        _server->offset(0);
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** EpixConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixConfigCache&  _cfg;
    EpixServer*      _server;
  unsigned           _result;
};

class EpixBeginCalibCycleAction : public Action {
  public:
    EpixBeginCalibCycleAction(EpixServer* s, EpixConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixBeginCalibCycleAction:;fire(tr) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and ");
          _server->offset(_server->offset()+_server->myCount()+1);
          printf(" offset %u count %u\n", _server->offset(), _server->myCount());
          if ((_result = _server->configure( (EpixConfigType*)_cfg.current()))) {
            printf("\nEpixBeginCalibCycleAction::fire(tr) failed config\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("EpixBeginCalibCycleAction:;fire(tr) enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** EpixConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    EpixServer*       _server;
    EpixConfigCache&  _cfg;
    unsigned          _result;
};


class EpixUnconfigAction : public Action {
 public:
   EpixUnconfigAction( EpixServer* server, EpixConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
   ~EpixUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("EpixUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("EpixUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epix Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   EpixServer*      _server;
   EpixConfigCache& _cfg;
   unsigned         _result;
};

class EpixEndCalibCycleAction : public Action {
  public:
    EpixEndCalibCycleAction(EpixServer* s, EpixConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** EpixEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixServer*      _server;
    EpixConfigCache& _cfg;
    unsigned          _result;
};

EpixManager::EpixManager( EpixServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new EpixConfigCache(server->client())) {

   printf("EpixManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int epix = open( devName,  O_RDWR | O_NONBLOCK);
   printf("pgpcard %s file number %d\n", devName, epix);
   if (epix < 0) {
     sprintf(err, "EpixManager::EpixManager() opening %s failed", devName);
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
     printf("EpixManager::EpixManager() illegal port mask!! 0x%x\n", ports);
   }

   server->manager(this);
   server->setEpix( epix );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new EpixAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new EpixUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new EpixConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new EpixEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new EpixDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new EpixBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new EpixEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new EpixL1Action( server ) );
   _fsm.callback( TransitionId::Unconfigure, new EpixUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new EpixBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new EpixEndRunAction( server ) );
}
