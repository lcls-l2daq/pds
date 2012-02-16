/*
 * PhasicsManager.cc
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
#include <time.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/PhasicsConfigType.hh"
#include "pds/camera/FrameType.hh"
#include "pds/phasics/PhasicsServer.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/phasics/PhasicsManager.hh"
#include "pds/phasics/PhasicsServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"

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
//      for (unsigned i=0; i<(sizeof(PhasicsConfigType)/sizeof(unsigned)); i++) {
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
//      int len = sizeof(PhasicsConfigType);
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
//    PhasicsConfigType _myconfig;
//  };
//

  class PhasicsConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      PhasicsConfigCache(const Src& src) :
        CfgCache(src, _PhasicsConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(PhasicsConfigType)) {}
    public:
     void printCurrent() {
       PhasicsConfigType* cfg = (PhasicsConfigType*)current();
       printf("PhasicsConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return sizeof(PhasicsConfigType); }
  };
}


using namespace Pds;

class PhasicsAllocAction : public Action {

  public:
   PhasicsAllocAction(PhasicsConfigCache& cfg, PhasicsServer* svr)
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
   PhasicsConfigCache& _cfg;
   PhasicsServer*      _svr;
};

class PhasicsUnmapAction : public Action {
  public:
    PhasicsUnmapAction(PhasicsServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      return tr;
    }

  private:
    PhasicsServer* server;
};

class PhasicsL1Action : public Action {
 public:
   PhasicsL1Action(PhasicsServer*, PhasicsManager*);

   InDatagram* fire(InDatagram* in);
   void        restart(bool r=false) {
     _restart = true;
     if (r) {
       _frameSyncError = false;
       _frameSyncErrorCount = 0;
     }
   }
   long long int timeDiff(timespec*, timespec*);
   bool        _restart;

   PhasicsServer*  server;
   PhasicsManager* _mgr;
   unsigned        _lastMatchedFiducial;
   bool            _frameSyncError;
   unsigned        _frameSyncErrorCount;
   timespec        _lastFrameTime;
   timespec        _lastEvrTime;
   GenericPool*    _occPool;
};

long long int PhasicsL1Action::timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

PhasicsL1Action::PhasicsL1Action(PhasicsServer* svr, PhasicsManager* mgr) :
    _restart(true),
    server(svr),
    _mgr(mgr),
    _lastMatchedFiducial(0xfffffff),
    _frameSyncError(false),
    _frameSyncErrorCount(0),
    _occPool(new GenericPool(sizeof(Occurrence),1))
{}

InDatagram* PhasicsL1Action::fire(InDatagram* in) {
  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
    if (server->debug() & 8) printf("PhasicsL1Action::fire! sec(%u) nsec(%u)\n", (unsigned)now.tv_sec, (unsigned)now.tv_nsec);
  if (in->datagram().xtc.damage.value() == 0) {
    Datagram& dg = in->datagram();
    timespec evrt;
    evrt.tv_sec   = dg.seq.clock().seconds();
    evrt.tv_nsec  = dg.seq.clock().nanoseconds();
    Xtc* xtc = &(dg.xtc);
    unsigned evrFiducials = dg.seq.stamp().fiducials();
    if (server->debug() & 8) {
      printf("\tsecs(%u) nano(%u) fiducials(0x%x)\n", (unsigned)evrt.tv_sec, (unsigned)evrt.tv_nsec, evrFiducials);
    }
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_Frame) {
        payload = xtc->payload();
      } else {
        printf("PhasicsL1Action::fire inner xtc not Id_Frame, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("PhasicsL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (_restart) {
      _restart = false;
    } else {
      long long int f = timeDiff(&server->timeStamp(), &_lastFrameTime);
      long long int e = timeDiff(&evrt, &_lastEvrTime);
      long long int d = llabs(f-e);
      if (d > 500000LL || _frameSyncError) {
        dg.xtc.damage.increase(Pds::Damage::UserDefined);
        dg.xtc.damage.userBits(0xb1);
        if (_frameSyncErrorCount++ < 10) {
          printf("PhasicsL1Action::fire setting user damage to frame %d due to off-by-one delta(%lld) frame(%lld) evr(%lld) nanoseconds\n",
              server->myCount(), d, f, e);
        }
        if (!_frameSyncError) server->printHisto(false);
        else _frameSyncError = true;
        Pds::Occurrence* occ = new (_occPool)
        Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
        _mgr->appliance().post(occ);
      }
    }

    memcpy(&_lastFrameTime, &server->timeStamp(), sizeof(timespec));
    memcpy(&_lastEvrTime, &evrt, sizeof(timespec));
  }
  return in;
}

class PhasicsConfigAction : public Action {

  public:
    PhasicsConfigAction( Pds::PhasicsConfigCache& cfg, PhasicsServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~PhasicsConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("PhasicsConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetCount();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (PhasicsConfigType*)_cfg.current())) && count++<1) {
          printf("\nPhasicsConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      }
      printf("PhasicsConfigAction::fire(Transition) returning transition\n");
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("PhasicsConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** PhasicsConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    PhasicsConfigCache&   _cfg;
    PhasicsServer*    _server;
  unsigned       _result;
};

class PhasicsBeginCalibCycleAction : public Action {
  public:
    PhasicsBeginCalibCycleAction(PhasicsServer* s, PhasicsConfigCache& cfg, PhasicsL1Action& l1) : _server(s), _cfg(cfg), _l1(l1), _result(0) {};

    Transition* fire(Transition* tr) {
      timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      printf("PhasicsBeginCalibCycleAction::fire(Transition) sec(%u) nsec(%u) ", (unsigned)now.tv_sec, (unsigned)now.tv_nsec);
      _l1.restart();
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
//          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (PhasicsConfigType*)_cfg.current())) && count++<1) {
            printf("\nPhasicsBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      printf("PhasicsBeginCalibCycleAction::fire(InDatagram) sec(%u) nsec(%u)", (unsigned)now.tv_sec, (unsigned)now.tv_nsec);
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** PhasicsConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    PhasicsServer*      _server;
    PhasicsConfigCache& _cfg;
    PhasicsL1Action&  _l1;
    unsigned          _result;
};


class PhasicsUnconfigAction : public Action {
 public:
   PhasicsUnconfigAction( PhasicsServer* server, PhasicsConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~PhasicsUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("PhasicsUnconfigAction::fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("PhasicsUnconfigAction::fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d phasics Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   PhasicsServer*      _server;
   PhasicsConfigCache& _cfg;
   unsigned          _result;
};

class PhasicsEndCalibCycleAction : public Action {
  public:
    PhasicsEndCalibCycleAction(PhasicsServer* s, PhasicsConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("PhasicsEndCalibCycleAction::fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      printf("\tdisabling front end\n");
      _server->disable();
      _result = _server->unconfigure();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("PhasicsEndCalibCycleAction::fire(InDatagram)\n");
      if( _result ) {
        printf( "*** PhasicsEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    PhasicsServer*      _server;
    PhasicsConfigCache& _cfg;
    unsigned          _result;
};

PhasicsManager::PhasicsManager( PhasicsServer* server) :
    _fsm(*new Fsm), _cfg(*new PhasicsConfigCache(server->client())) {

   printf("PhasicsManager being initialized... " );
   PhasicsL1Action* l1 = new PhasicsL1Action( server, this );
   _fsm.callback( TransitionId::L1Accept, l1 );

   _fsm.callback( TransitionId::Map, new PhasicsAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new PhasicsUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new PhasicsConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new PhasicsEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new PhasicsDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new PhasicsBeginCalibCycleAction( server, _cfg, *l1 ) );
   _fsm.callback( TransitionId::EndCalibCycle, new PhasicsEndCalibCycleAction( server, _cfg ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new PhasicsEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new PhasicsUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new PhasicsBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new PhasicsEndRunAction( server ) );
}
