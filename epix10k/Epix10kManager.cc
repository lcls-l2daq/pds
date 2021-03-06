/*
 * Epix10kManager.cc
 *
 *  Created on: 2014.4.15
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
#include "pds/config/EpixDataType.hh"
#include "pds/epix10k/Epix10kServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/epix10k/Epix10kManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;


  class Epix10kConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      Epix10kConfigCache(const Src& src) :
        CfgCache(src, _epix10kConfigType, StaticALlocationNumberOfConfigurationsForScanning * __size()) {}
    public:
     void printCurrent() {
       Epix10kConfigType* cfg = (Epix10kConfigType*)current();
       printf("Epix10kConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return ((Epix10kConfigType*)tc)->_sizeof(); }
      static int __size() {
        Epix10kConfigType* foo = new Epix10kConfigType(2,2,0xb0, 0xc0, 2);
        int size = (int) foo->_sizeof();
        delete foo;
        return size;
      }
  };

  class myIterator : public XtcIterator {
  public:
    myIterator() : _result(0), _elem(0) {}
  public:
    const EpixDataType*   elem() const { return _elem; }
  public:
    int process(Xtc* xtc) {  // return 0 to stop iterating
      _result = 1;
      switch(xtc->contains.id()) {
        case TypeId::Id_Xtc :
          iterate(xtc);      // iterate down another level
          return _result;
        case TypeId::Id_EpixElement:
          _elem = reinterpret_cast<const EpixDataType*>(xtc->payload());
          _result = 0;
          break;
        default:
          break;
      }
      return _result;
    }
  private:
    int _result;
    const EpixDataType*   _elem;
  };





class Epix10kAllocAction : public Action {

  public:
   Epix10kAllocAction(Epix10kConfigCache& cfg, Epix10kServer* svr)
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
   Epix10kConfigCache& _cfg;
   Epix10kServer*      _svr;
};

class Epix10kUnmapAction : public Action {
  public:
    Epix10kUnmapAction(Epix10kServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("Epix10kUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    Epix10kServer* server;
};

class Epix10kL1Action : public Action {
 public:
   Epix10kL1Action(Epix10kServer* svr);

   InDatagram* fire(InDatagram* in);
   void        reset();

   Epix10kServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _lastMatchedAcqCount;
   unsigned _printCount;
   bool     _fiducialError;
};

void Epix10kL1Action::reset() {
  printf("Epix10kL1Action::reset()\n");
  _lastMatchedFiducial = 0xfffffff;
  _lastMatchedAcqCount = 0xfffffff;
  _fiducialError       = false;
}

Epix10kL1Action::Epix10kL1Action(Epix10kServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _lastMatchedAcqCount(0xffffffff),
    _printCount(0),
    _fiducialError(false) {}

InDatagram* Epix10kL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("Epix10kL1Action::fire!\n");
  Datagram& dg = in->datagram();
  if (in->datagram().xtc.damage.value() == 0) {
    unsigned evrFiducial = dg.seq.stamp().fiducials();
    const EpixDataType* data;
//    printf(" ->%d ", evrFiducial);
    if (_fiducialError == false) {
      myIterator iter;
      iter.iterate(&dg.xtc);  // find the element object
      if ((data = iter.elem())) {
        unsigned acqCount = data->acqCount();
        if ( (evrFiducial > _lastMatchedFiducial) && (acqCount > _lastMatchedAcqCount) ) {
          unsigned fidDiff = evrFiducial - _lastMatchedFiducial;
          unsigned acqDiff = acqCount     - _lastMatchedAcqCount;
          if (fidDiff/3 != acqDiff) {
            dg.xtc.damage.increase(Pds::Damage::UserDefined);
            dg.xtc.damage.userBits(0xfd);
            if (_printCount++<32) {
              printf("Epix10kL1Action setting user damage delta fiducial %x and acq %x, last %x %x values %x %x vector %x\n",
                  fidDiff/3, acqDiff, _lastMatchedFiducial, _lastMatchedAcqCount, evrFiducial, acqCount, dg.seq.stamp().vector());
            }
            if (!_fiducialError) {
//              server->printHisto(false);
//              _fiducialError = true;
            }
          } else {
//            printf(".");
            _printCount = 0;
            _lastMatchedFiducial = evrFiducial;
            _lastMatchedAcqCount = acqCount;
          }
        } else {
          printf("Epix10kL1Action initial or wrapping fid %x acqCount %x\n", evrFiducial, acqCount);
          _lastMatchedFiducial = evrFiducial;
          _lastMatchedAcqCount = acqCount;
        }
      } else {
        printf("Epix10kL1Action::fire iterator could not find _epixDataType\n");
      }
    } else {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xfd);
    }
  }
  return in;
}

class Epix10kConfigAction : public Action {

  public:
    Epix10kConfigAction( Pds::Epix10kConfigCache& cfg, Epix10kServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~Epix10kConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("Epix10kConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        if ((_result = _server->configure( (Epix10kConfigType*)_cfg.current()))) {
          printf("\nEpix10kConfigAction::fire(tr) failed configuration\n");
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else {
        _server->offset(0);
      }
      _server->manager()->l1Action->reset();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->samplerConfig()) {
        printf("Epix10kConfigAction sending sampler config\n");
        in->insert(_server->xtcConfig(), _server->samplerConfig());
      } else {
        printf("Epix10kConfigAction not sending sampler config\n");
      }
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** Epix10kConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    Epix10kConfigCache&  _cfg;
    Epix10kServer*      _server;
  unsigned           _result;
};

class Epix10kBeginCalibCycleAction : public Action {
  public:
    Epix10kBeginCalibCycleAction(Epix10kServer* s, Epix10kConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("Epix10kBeginCalibCycleAction:;fire(tr) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and ");
          _server->offset(_server->offset()+_server->myCount()+1);
          printf(" offset %u count %u\n", _server->offset(), _server->myCount());
          if ((_result = _server->configure( (Epix10kConfigType*)_cfg.current()))) {
            printf("\nEpix10kBeginCalibCycleAction::fire(tr) failed config\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("Epix10kBeginCalibCycleAction:;fire(tr) enabled\n");
      _server->latchAcqCount();
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->samplerConfig()) {
          printf("Epix10kBeginCalibCycleAction sending sampler config\n");
          in->insert(_server->xtcConfig(), _server->samplerConfig());
        } else {
          printf("Epix10kBeginCalibCycleAction not sending sampler config\n");
        }
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result ) {
        printf( "*** Epix10kConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    Epix10kServer*       _server;
    Epix10kConfigCache&  _cfg;
    unsigned          _result;
};


class Epix10kUnconfigAction : public Action {
 public:
   Epix10kUnconfigAction( Epix10kServer* server, Epix10kConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
   ~Epix10kUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("Epix10kUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("Epix10kUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d epix10k Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   Epix10kServer*      _server;
   Epix10kConfigCache& _cfg;
   unsigned         _result;
};

class Epix10kEndCalibCycleAction : public Action {
  public:
    Epix10kEndCalibCycleAction(Epix10kServer* s, Epix10kConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("Epix10kEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      printf("Epix10kEndCalibCycleAction tr acqCounts this cycle %d\n", 1 + _server->lastAcqCount() - _server->latchedAcqCount());
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Epix10kEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** Epix10kEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    Epix10kServer*      _server;
    Epix10kConfigCache& _cfg;
    unsigned          _result;
};
}

using namespace Pds;

Epix10kManager::Epix10kManager( Epix10kServer* server, unsigned d) :
    _fsm(*new Fsm), _cfg(*new Epix10kConfigCache(server->client())) {

   printf("Epix10kManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int epix10k = open( devName,  O_RDWR | O_NONBLOCK);
   printf("pgpcard %s file number %d\n", devName, epix10k);
   if (epix10k < 0) {
     sprintf(err, "Epix10kManager::Epix10kManager() opening %s failed", devName);
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
     printf("Epix10kManager::Epix10kManager() illegal port mask!! 0x%x\n", ports);
   }

   server->manager(this);
   server->setEpix10k( epix10k );
//   server->laneTest();

   l1Action = new Epix10kL1Action( server );

   _fsm.callback( TransitionId::Map, new Epix10kAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new Epix10kUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new Epix10kConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new Epix10kEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new Epix10kDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new Epix10kBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new Epix10kEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, l1Action );
   _fsm.callback( TransitionId::Unconfigure, new Epix10kUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new Epix10kBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new Epix10kEndRunAction( server ) );
}
