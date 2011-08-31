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
#include "pds/cspad/CspadServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/cspad/CspadManager.hh"
#include "pds/cspad/CspadServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  class CspadConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      CspadConfigCache(const Src& src) :
        CfgCache(src, _CsPadConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(CsPadConfigType)) {}
    public:
     void printRO() {
       CsPadConfigType* cfg = (CsPadConfigType*)current();
       printf("CspadConfigCache::printRO concentrator config 0x%x\n", (unsigned)cfg->concentratorVersion());
     }
    private:
      int _size(void* tc) const { return sizeof(CsPadConfigType); }
  };
}


using namespace Pds;

class CspadAllocAction : public Action {

  public:
   CspadAllocAction(CspadConfigCache& cfg)
      : _cfg(cfg) {}

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.init(alloc.allocation());
      return tr;
   }

 private:
   CspadConfigCache& _cfg;
};

class CspadUnmapAction : public Action {
  public:
    CspadUnmapAction(CspadServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("CspadUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    CspadServer* server;
};

class CspadL1Action : public Action {
 public:
   CspadL1Action(CspadServer* svr);

   InDatagram* fire(InDatagram* in);
   void        reset(bool f=true);

   enum {FiducialErrorCountLimit=16};

   CspadServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _lastMatchedFrameNumber;
   unsigned _lastMatchedAcqCount;
   unsigned _frameSyncErrorCount;
   unsigned _ioIndex;
   unsigned _resetCount;

};

void CspadL1Action::reset(bool resetError) {
  _lastMatchedFiducial = 0xffffffff;
  _lastMatchedFrameNumber = 0xffffffff;
  if (resetError) _frameSyncErrorCount = 0;
}

CspadL1Action::CspadL1Action(CspadServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xffffffff),
    _lastMatchedFrameNumber(0xffffffff),
    _frameSyncErrorCount(0),
    _resetCount(0) {}

InDatagram* CspadL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("CspadL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned frameError = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if ((xtc->contains.id() == Pds::TypeId::Id_CspadElement) ||
          (xtc->contains.id() == Pds::TypeId::Id_Cspad2x2Element)) {
        payload = xtc->payload();
      } else {
        printf("CspadLiAction::fire inner xtc not Id_Cspad[2x2]Element, but %s!\n",
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
      if (server->payloadSize() > 500000) {     // test if this is no the mini 2x2
        if (evrFiducials != data->fiducials()) {
          frameError |= 1<<data->elementId();
          if (_frameSyncErrorCount < FiducialErrorCountLimit) {
            printf("CspadL1Action::fire(in) fiducial mismatch evr(0x%x) cspad(0x%x) in quad %u,%u lastMatchedFiducial(0x%x)\n\tframeNumber(0x%x), lastMatchedFrameNumber(0x%x), ",
                evrFiducials, data->fiducials(), data->elementId(), i, _lastMatchedFiducial, data->frameNumber(), _lastMatchedFrameNumber);
            printf("acqCount(0x%x), lastMatchedAcqCount(0x%x)\n", data->acqCount(), _lastMatchedAcqCount);
          }
        }
      } else {
        bool noWrap = true;
        unsigned acq = data->acqCount();
        if ((acq < _lastMatchedAcqCount) || (evrFiducials < _lastMatchedFiducial)) {
          noWrap = false;
          _resetCount += 1;
        }
        if (evrFiducials == _lastMatchedFiducial) _resetCount += 4;
        if (_resetCount)  {
          reset(false);
          if (server->debug() & 0x80) printf("CspadL1Action::reset(%u) evrFiducials(0x%x) acq(0x%x)\n", _resetCount, evrFiducials, acq);
          _resetCount--;
          noWrap = false;
        }
        if (noWrap && ((evrFiducials-_lastMatchedFiducial) != (3 * (acq-_lastMatchedAcqCount)))) {
          frameError |= 1;
          if (_frameSyncErrorCount < FiducialErrorCountLimit) {
            printf("CspadL1Action::fire(in) frame mismatch!  evr(0x%x) lastMatchedFiducial(0x%x)\n\tframeNumber(0x%x), lastMatchedFrameNumber(0x%x), ",
                evrFiducials, _lastMatchedFiducial, data->frameNumber(), _lastMatchedFrameNumber);
            printf("acqCount(0x%x), lastMatchedAcqCount(0x%x)\n", acq, _lastMatchedAcqCount);
          }
        }
        _lastMatchedFiducial = evrFiducials;
        _lastMatchedAcqCount = acq;
        if (_frameSyncErrorCount) frameError |= 1;
      }
      if (!frameError) {
        _lastMatchedFiducial = evrFiducials;
        _lastMatchedFrameNumber = data->frameNumber();
        _lastMatchedAcqCount = data->acqCount();
        _frameSyncErrorCount = 0;
      }
      if (server->debug() & 0x40) printf("L1 acq - frm# %d\n", data->acqCount() - data->frameNumber());
      // Kludge test of sending nothing ....                  !!!!!!!!!!!!!!!!!!!!!!
//      if (data->frameNumber()) {
//        if ((data->frameNumber() & 0xfff) == 0) {
//          printf("CspadL1Action::CspadL1Action sending nothing %u 0x%x\n",
//              data->frameNumber(),
//              data->fiducials());
//          return NULL;
//        }
//      }
    }
    if (frameError) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (frameError&0xf));
      if (_frameSyncErrorCount++ < FiducialErrorCountLimit) {
        printf("CspadL1Action setting user damage due to frame error in quads(0x%x)\n", frameError);
        if (!_frameSyncErrorCount) server->printHisto(false);
      } else {
        if (_frameSyncErrorCount == FiducialErrorCountLimit) {
          printf("CspadL1Action::fire(in) is turning of frame error reporting until we see a match\n");
        }
      }
    }
    if (!frameError) {
      server->process();
    }
  }
  return in;
}

class CspadConfigAction : public Action {

  public:
    CspadConfigAction( Pds::CspadConfigCache& cfg, CspadServer* server, CspadL1Action& l1)
    : _cfg( cfg ), _server(server), _l1(l1), _result(0)
      {}

    ~CspadConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("CspadConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      _l1.reset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (CsPadConfigType*)_cfg.current())) && count++<10) {
          printf("\nCspadConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printRO();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("CspadConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printRO();
      if( _result ) {
        printf( "*** CspadConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    CspadConfigCache&   _cfg;
    CspadServer*    _server;
    CspadL1Action&  _l1;
  unsigned       _result;
};

class CspadBeginCalibCycleAction : public Action {
  public:
    CspadBeginCalibCycleAction(CspadServer* s, CspadConfigCache& cfg, CspadL1Action& l1) : _server(s), _cfg(cfg), _l1(l1), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("CspadBeginCalibCycleAction:;fire(Transition) payload size %u ", _server->payloadSize());
      _l1.reset();
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (CsPadConfigType*)_cfg.current())) && count++<10) {
            printf("\nCspadBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printRO();
        }
      }
      printf("enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("CspadBeginCalibCycleAction:;fire(InDatagram) payload size %u ", _server->payloadSize());
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printRO();
      } else printf("\n");
      if( _result ) {
        printf( "*** CspadConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }
  private:
    CspadServer*      _server;
    CspadConfigCache& _cfg;
    CspadL1Action&    _l1;
    unsigned          _result;
};


class CspadUnconfigAction : public Action {
 public:
   CspadUnconfigAction( CspadServer* server, CspadConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~CspadUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("CspadUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("CspadUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d cspad Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   CspadServer*      _server;
   CspadConfigCache& _cfg;
   unsigned          _result;
};

class CspadEndCalibCycleAction : public Action {
  public:
    CspadEndCalibCycleAction(CspadServer* s, CspadConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("CspadEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("CspadEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** CspadEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    CspadServer*      _server;
    CspadConfigCache& _cfg;
    unsigned          _result;
};

CspadManager::CspadManager( CspadServer* server) :
    _fsm(*new Fsm), _cfg(*new CspadConfigCache(server->client())) {

   printf("CspadManager being initialized... " );

   int cspad = open( "/dev/pgpcard",  O_RDWR);
   printf("pgpcard file number %d\n", cspad);
   if (cspad < 0) {
     perror("CspadManager::CspadManager() opening pgpcard failed");
     // What else to do if the open fails?
     ::exit(-1);
   }

   server->setCspad( cspad );

   CspadL1Action* l1 = new CspadL1Action( server );
   _fsm.callback( TransitionId::L1Accept, l1 );

   _fsm.callback( TransitionId::Map, new CspadAllocAction( _cfg ) );
   _fsm.callback( TransitionId::Unmap, new CspadUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new CspadConfigAction(_cfg, server, *l1 ) );
   //   _fsm.callback( TransitionId::Enable, new CspadEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new CspadDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new CspadBeginCalibCycleAction( server, _cfg, *l1 ) );
   _fsm.callback( TransitionId::EndCalibCycle, new CspadEndCalibCycleAction( server, _cfg ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new CspadEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new CspadUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new CspadBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new CspadEndRunAction( server ) );
}
