/*
 * FexampManager.cc
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
#include "pds/config/FexampConfigType.hh"
#include "pds/fexamp/FexampServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/fexamp/FexampManager.hh"
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
//      for (unsigned i=0; i<(sizeof(FexampConfigType)/sizeof(unsigned)); i++) {
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
//      int len = sizeof(FexampConfigType);
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
//    FexampConfigType _myconfig;
//  };
//

  class FexampConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      FexampConfigCache(const Src& src) :
        CfgCache(src, _FexampConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(FexampConfigType)) {}
    public:
     void printCurrent() {
       FexampConfigType* cfg = (FexampConfigType*)current();
       printf("FexampConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return sizeof(FexampConfigType); }
  };
}


using namespace Pds;

class FexampAllocAction : public Action {

  public:
   FexampAllocAction(FexampConfigCache& cfg, FexampServer* svr)
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
   FexampConfigCache& _cfg;
   FexampServer*      _svr;
};

class FexampUnmapAction : public Action {
  public:
    FexampUnmapAction(FexampServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("FexampUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    FexampServer* server;
};

class FexampL1Action : public Action {
 public:
   FexampL1Action(FexampServer* svr);

   InDatagram* fire(InDatagram* in);

   FexampServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _ioIndex;
   bool     _fiducialError;

};

FexampL1Action::FexampL1Action(FexampServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* FexampL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("FexampL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_FexampElement) {
        payload = xtc->payload();
      } else {
        printf("FexampL1Action::fire inner xtc not Id_FexampElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("FexampL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("FexampL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class FexampConfigAction : public Action {

  public:
    FexampConfigAction( Pds::FexampConfigCache& cfg, FexampServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~FexampConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("FexampConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (FexampConfigType*)_cfg.current())) && count++<10) {
          printf("\nFexampConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("FexampConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** FexampConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    FexampConfigCache&   _cfg;
    FexampServer*    _server;
  unsigned       _result;
};

class FexampBeginCalibCycleAction : public Action {
  public:
    FexampBeginCalibCycleAction(FexampServer* s, FexampConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("FexampBeginCalibCycleAction:;fire(Transition) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
//          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (FexampConfigType*)_cfg.current())) && count++<10) {
            printf("\nFexampBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("FexampBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** FexampConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    FexampServer*      _server;
    FexampConfigCache& _cfg;
    unsigned          _result;
};


class FexampUnconfigAction : public Action {
 public:
   FexampUnconfigAction( FexampServer* server, FexampConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~FexampUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("FexampUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("FexampUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d fexamp Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   FexampServer*      _server;
   FexampConfigCache& _cfg;
   unsigned          _result;
};

class FexampEndCalibCycleAction : public Action {
  public:
    FexampEndCalibCycleAction(FexampServer* s, FexampConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("FexampEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("FexampEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** FexampEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    FexampServer*      _server;
    FexampConfigCache& _cfg;
    unsigned          _result;
};

FexampManager::FexampManager( FexampServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new FexampConfigCache(server->client())) {

   printf("FexampManager being initialized... " );

   char devName[128];
   char err[128];
   sprintf(devName, "/dev/pgpcard%u", d);

   int fexamp = open( devName,  O_RDWR);
   printf("pgpcard file number %d\n", fexamp);
   if (fexamp < 0) {
     sprintf(err, "FexampManager::FexampManager() opening %s failed", devName);
     perror(err);
     // What else to do if the open fails?
     ::exit(-1);
   }

   server->setFexamp( fexamp );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new FexampAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new FexampUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new FexampConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new FexampEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new FexampDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new FexampBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new FexampEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new FexampL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new FexampEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new FexampUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new FexampBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new FexampEndRunAction( server ) );
}
