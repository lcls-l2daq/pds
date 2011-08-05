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

   enum {FiducialErrorCountLimit=4};

   CspadServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _lastMatchedFrameNumber;
   unsigned _fiducialErrorCount;
   unsigned _ioIndex;

};

CspadL1Action::CspadL1Action(CspadServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _lastMatchedFrameNumber(0xfffffff),
    _fiducialErrorCount(0) {}

InDatagram* CspadL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("CspadL1Action::fire!\n");
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
      if (data->fiducials()) {
        if (evrFiducials != data->fiducials()) {
          error |= 1<<data->elementId();
          if (_fiducialErrorCount < FiducialErrorCountLimit) {
            printf("CspadL1Action::fire(in) fiducial mismatch evr(0x%x) cspad(0x%x) in quad %u,%u lastMatchedFiducial(0x%x), frameNumber(%u), lastMatchedFrameNumber(%u)\n",
                evrFiducials, data->fiducials(), data->elementId(), i, _lastMatchedFiducial, data->frameNumber(), _lastMatchedFrameNumber);
          }
        } else {
          _lastMatchedFiducial = evrFiducials;
          _lastMatchedFrameNumber = data->frameNumber();
          _fiducialErrorCount = 0;
        }
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
    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      if (_fiducialErrorCount < FiducialErrorCountLimit) {
        printf("CspadL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
        if (!_fiducialErrorCount++) server->printHisto(false);
      }
    }
    if (!error) {
      server->process();
    }
  }
  return in;
}

class CspadConfigAction : public Action {

  public:
    CspadConfigAction( Pds::CspadConfigCache& cfg, CspadServer* server)
    : _cfg( cfg ), _server(server), _result(0)
      {}

    ~CspadConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("CspadConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
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
  unsigned       _result;
};

class CspadBeginCalibCycleAction : public Action {
  public:
    CspadBeginCalibCycleAction(CspadServer* s, CspadConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("CspadBeginCalibCycleAction:;fire(Transition) ");
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
      printf("CspadBeginCalibCycleAction:;fire(InDatagram)");
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

   _fsm.callback( TransitionId::Map, new CspadAllocAction( _cfg ) );
   _fsm.callback( TransitionId::Unmap, new CspadUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new CspadConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new CspadEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new CspadDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new CspadBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new CspadEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new CspadL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new CspadEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new CspadUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new CspadBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new CspadEndRunAction( server ) );
}
