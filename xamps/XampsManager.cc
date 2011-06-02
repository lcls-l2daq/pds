/*
 * XampsManager.cc
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
#include "pds/config/XampsConfigType.hh"
#include "pds/xamps/XampsServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/xamps/XampsManager.hh"
#include "pds/xamps/XampsServer.hh"
#include "pds/pgp/DataImportFrame.hh"
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
//      for (unsigned i=0; i<(sizeof(XampsConfigType)/sizeof(unsigned)); i++) {
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
//      int len = sizeof(XampsConfigType);
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
//    XampsConfigType _myconfig;
//  };
//

  class XampsConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      XampsConfigCache(const Src& src) :
        CfgCache(src, _XampsConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(XampsConfigType)) {}
    public:
     void printCurrent() {
       XampsConfigType* cfg = (XampsConfigType*)current();
       printf("XampsConfigCache::printCurrent current 0x%x\n", (unsigned)cfg);
     }
    private:
      int _size(void* tc) const { return sizeof(XampsConfigType); }
  };
}


using namespace Pds;

class XampsAllocAction : public Action {

  public:
   XampsAllocAction(XampsConfigCache& cfg)
      : _cfg(cfg) {}

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.init(alloc.allocation());
      return tr;
   }

 private:
   XampsConfigCache& _cfg;
};

class XampsUnmapAction : public Action {
  public:
    XampsUnmapAction(XampsServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("XampsUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    XampsServer* server;
};

class XampsL1Action : public Action {
 public:
   XampsL1Action(XampsServer* svr);

   InDatagram* fire(InDatagram* in);

   XampsServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _ioIndex;
   bool     _fiducialError;

};

XampsL1Action::XampsL1Action(XampsServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* XampsL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("XampsL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_XampsElement) {
        payload = xtc->payload();
      } else {
        printf("XampsL1Action::fire inner xtc not Id_XampsElement, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("XampsL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

//    for (unsigned i=0; i<server->numberOfQuads(); i++) {
//      data = (Pds::Pgp::DataImportFrame*) ( payload + (i * server->payloadSize()) );
//      if (evrFiducials != data->fiducials()) {
//        error |= 1<<i;
//        printf("XampsL1Action::fire(in) fiducial mismatch evr(0x%x) xamps(0x%x) in quad %u, lastMatchedFiducial(0x%x), frameNumber(%u)\n",
//            evrFiducials, data->fiducials(), i, _lastMatchedFiducial, data->frameNumber());
//      } else {
//        _lastMatchedFiducial = evrFiducials;
//      }
//      // Kludge test of sending nothing ....                  !!!!!!!!!!!!!!!!!!!!!!
////      if (data->frameNumber()) {
////        if ((data->frameNumber() & 0xfff) == 0) {
////          printf("XampsL1Action::XampsL1Action sending nothing %u 0x%x\n",
////              data->frameNumber(),
////              data->fiducials());
////          return NULL;
////        }
////      }
//    }
    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("XampsL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
      server->process();
    }
  }
  return in;
}

class XampsConfigAction : public Action {

  public:
    XampsConfigAction( Pds::XampsConfigCache& cfg, XampsServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~XampsConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("XampsConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (XampsConfigType*)_cfg.current())) && count++<10) {
          printf("\nXampsConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("XampsConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** XampsConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    XampsConfigCache&   _cfg;
    XampsServer*    _server;
  unsigned       _result;
};

class XampsBeginCalibCycleAction : public Action {
  public:
    XampsBeginCalibCycleAction(XampsServer* s, XampsConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("XampsBeginCalibCycleAction:;fire(Transition) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (XampsConfigType*)_cfg.current())) && count++<10) {
            printf("\nXampsBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("XampsBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** XampsConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    XampsServer*      _server;
    XampsConfigCache& _cfg;
    unsigned          _result;
};


class XampsUnconfigAction : public Action {
 public:
   XampsUnconfigAction( XampsServer* server, XampsConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~XampsUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("XampsUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("XampsUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d xamps Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   XampsServer*      _server;
   XampsConfigCache& _cfg;
   unsigned          _result;
};

class XampsEndCalibCycleAction : public Action {
  public:
    XampsEndCalibCycleAction(XampsServer* s, XampsConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("XampsEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("XampsEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** XampsEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    XampsServer*      _server;
    XampsConfigCache& _cfg;
    unsigned          _result;
};

XampsManager::XampsManager( XampsServer* server) :
    _fsm(*new Fsm), _cfg(*new XampsConfigCache(server->client())) {

   printf("XampsManager being initialized... " );

   int xamps = open( "/dev/pgpcard",  O_RDWR);
   printf("pgpcard file number %d\n", xamps);
   if (xamps < 0) {
     perror("XampsManager::XampsManager() opening pgpcard failed");
     // What else to do if the open fails?
     ::exit(-1);
   }

   server->setXamps( xamps );
   server->laneTest();

   _fsm.callback( TransitionId::Map, new XampsAllocAction( _cfg ) );
   _fsm.callback( TransitionId::Unmap, new XampsUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new XampsConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new XampsEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new XampsDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new XampsBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new XampsEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new XampsL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new XampsEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new XampsUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new XampsBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new XampsEndRunAction( server ) );
}
