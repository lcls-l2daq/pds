/*
 * EpixSamplerManager.cc
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
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/epixSampler/EpixSamplerServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/epixSampler/EpixSamplerManager.hh"
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
//      for (unsigned i=0; i<(sizeof(EpixSamplerConfigType)/sizeof(unsigned)); i++) {
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
//      int len = sizeof(EpixSamplerConfigType);
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
//    EpixSamplerConfigType _myconfig;
//  };
//

  class EpixSamplerConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      EpixSamplerConfigCache(const Src& src) :
        CfgCache(src, _epixSamplerConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(EpixSamplerConfigType)) {}
    public:
     void printCurrent() {
       EpixSamplerConfigType* cfg = (EpixSamplerConfigType*)current();
       printf("EpixSamplerConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return sizeof(EpixSamplerConfigType); }
  };
}


using namespace Pds;

class EpixSamplerAllocAction : public Action {

  public:
   EpixSamplerAllocAction(EpixSamplerConfigCache& cfg, EpixSamplerServer* svr)
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
   EpixSamplerConfigCache& _cfg;
   EpixSamplerServer*      _svr;
};

class EpixSamplerUnmapAction : public Action {
  public:
    EpixSamplerUnmapAction(EpixSamplerServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("EpixSamplerUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    EpixSamplerServer* server;
};

class EpixSamplerL1Action : public Action {
 public:
   EpixSamplerL1Action(EpixSamplerServer* svr);

   InDatagram* fire(InDatagram* in);

   EpixSamplerServer* server;
   unsigned _lastMatchedFiducial;
   bool     _fiducialError;

};

EpixSamplerL1Action::EpixSamplerL1Action(EpixSamplerServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* EpixSamplerL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("EpixSamplerL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_EpixSamplerElement) {
        payload = xtc->payload();
      } else {
        printf("EpixSamplerL1Action::fire inner xtc not Id_EpixSamplerElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("EpixSamplerL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("EpixSamplerL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class EpixSamplerConfigAction : public Action {

  public:
    EpixSamplerConfigAction( Pds::EpixSamplerConfigCache& cfg, EpixSamplerServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~EpixSamplerConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("EpixSamplerConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (EpixSamplerConfigType*)_cfg.current())) && count++<10) {
          printf("\nEpixSamplerConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSamplerConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** EpixSamplerConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixSamplerConfigCache&   _cfg;
    EpixSamplerServer*    _server;
  unsigned       _result;
};

class EpixSamplerBeginCalibCycleAction : public Action {
  public:
    EpixSamplerBeginCalibCycleAction(EpixSamplerServer* s, EpixSamplerConfigCache& cfg) :
      _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixSamplerBeginCalibCycleAction:;fire(Transition) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
//          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (EpixSamplerConfigType*)_cfg.current())) && count++<10) {
            printf("\nEpixSamplerBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSamplerBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** EpixSamplerConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    EpixSamplerServer*      _server;
    EpixSamplerConfigCache& _cfg;
    unsigned          _result;
};


class EpixSamplerUnconfigAction : public Action {
 public:
   EpixSamplerUnconfigAction( EpixSamplerServer* server, EpixSamplerConfigCache& cfg ) :
     _server( server ), _cfg(cfg), _result(0) {}
   ~EpixSamplerUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("EpixSamplerUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("EpixSamplerUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epixSampler Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   EpixSamplerServer*      _server;
   EpixSamplerConfigCache& _cfg;
   unsigned          _result;
};

class EpixSamplerEnableAction : public Action {
 public:
   EpixSamplerEnableAction( EpixSamplerServer* server) : _server( server ) {}
   ~EpixSamplerEnableAction() {}

   Transition* fire(Transition* tr) {
     printf("EpixSamplerEnableAction:;fire(Transition) enabled\n");
     _server->enable();
     _server->dumpFrontEnd();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     return in;
   }

 private:
   EpixSamplerServer*      _server;
};

class EpixSamplerDisableAction : public Action {
 public:
   EpixSamplerDisableAction( EpixSamplerServer* server) : _server(server) {}
   ~EpixSamplerDisableAction() {}

   Transition* fire(Transition* tr) {
     printf("EpixSamplerDisableAction:;fire(Transition) disabled\n");
     _server->disable();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {return in;}

 private:
   EpixSamplerServer*      _server;
};

class EpixSamplerEndCalibCycleAction : public Action {
  public:
    EpixSamplerEndCalibCycleAction(EpixSamplerServer* s, EpixSamplerConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("EpixSamplerEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("EpixSamplerEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** EpixSamplerEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    EpixSamplerServer*      _server;
    EpixSamplerConfigCache& _cfg;
    unsigned          _result;
};

EpixSamplerManager::EpixSamplerManager( EpixSamplerServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new EpixSamplerConfigCache(server->client())) {

   printf("EpixSamplerManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int epixSampler = open( devName,  O_RDWR);
   printf("pgpcard file number %d\n pgpcard number %u, portmask 0x%x", epixSampler, d & 0xf, ports);
   if (epixSampler < 0) {
     sprintf(err, "EpixSamplerManager::EpixSamplerManager() opening %s failed", devName);
     perror(err);
     // What else to do if the open fails?
     ::exit(-1);
   }

   server->setEpixSampler( epixSampler );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new EpixSamplerAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new EpixSamplerUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new EpixSamplerConfigAction(_cfg, server ) );
   _fsm.callback( TransitionId::Enable, new EpixSamplerEnableAction( server ) );
   _fsm.callback( TransitionId::Disable, new EpixSamplerDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new EpixSamplerBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new EpixSamplerEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new EpixSamplerL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new EpixSamplerEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new EpixSamplerUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new EpixSamplerBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new EpixSamplerEndRunAction( server ) );
}
