/*
 * Cspad2x2Manager.cc
 *
 *  Created on: Jan 9, 2012
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
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/cspad2x2/Cspad2x2Server.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/cspad2x2/Cspad2x2Manager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/config/CfgCache.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  class Cspad2x2ConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      Cspad2x2ConfigCache(const Src& src) :
        CfgCache(src, _CsPad2x2ConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(CsPad2x2ConfigType)) {}
    public:
     void printRO() {
       CsPad2x2ConfigType* cfg = (CsPad2x2ConfigType*)current();
       printf("Cspad2x2ConfigCache::printRO concentrator config 0x%x\n", (unsigned)cfg->concentratorVersion());
     }
    private:
      int _size(void* tc) const { return sizeof(CsPad2x2ConfigType); }
  };
}


using namespace Pds;

class Cspad2x2AllocAction : public Action {

  public:
   Cspad2x2AllocAction(Cspad2x2ConfigCache& cfg)
      : _cfg(cfg) {}

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.init(alloc.allocation());
      return tr;
   }

 private:
   Cspad2x2ConfigCache& _cfg;
};

class Cspad2x2UnmapAction : public Action {
  public:
    Cspad2x2UnmapAction(Cspad2x2Server* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("Cspad2x2UnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    Cspad2x2Server* server;
};

class Cspad2x2L1Action : public Action {
 public:
   Cspad2x2L1Action(Cspad2x2Server* svr);

   InDatagram* fire(InDatagram* in);
   void        reset(bool f=true);

   enum {FiducialErrorCountLimit=16};

   Cspad2x2Server* server;
   unsigned _lastMatchedFiducial;
   unsigned _lastMatchedFrameNumber;
   unsigned _lastMatchedAcqCount;
   unsigned _frameSyncErrorCount;
   unsigned _damageCount;
};

void Cspad2x2L1Action::reset(bool resetError) {
  _lastMatchedFiducial = 0xffffffff;
  _lastMatchedFrameNumber = 0xffffffff;
  if (resetError) _frameSyncErrorCount = 0;
}

Cspad2x2L1Action::Cspad2x2L1Action(Cspad2x2Server* svr) :
    server(svr),
    _lastMatchedFiducial(0xffffffff),
    _lastMatchedFrameNumber(0xffffffff),
    _frameSyncErrorCount(0),
    _damageCount(0) {}

InDatagram* Cspad2x2L1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("Cspad2x2L1Action::fire!\n");
  Datagram& dg = in->datagram();
  Xtc* xtc = &(dg.xtc);
  unsigned evrFiducials = dg.seq.stamp().fiducials();
  unsigned vector = dg.seq.stamp().vector();
  if (in->datagram().xtc.damage.value() != 0) {
    if (_damageCount++ < 32) {
      printf("Cspad2x2L1Action::fire damage(0x%x), fiducials(0x%x), vector(0x%x)\n",
          in->datagram().xtc.damage.value(), evrFiducials, vector);
    }
  } else {
    Pds::Pgp::DataImportFrame* data;
    unsigned frameError = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_Cspad2x2Element) {
        payload = xtc->payload();
      } else {
        printf("Cspad2x2L1Action::fire inner xtc not Id_Cspad2x2Element, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("Cspad2x2L1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    data = (Pds::Pgp::DataImportFrame*) payload;
    bool noWrap = true;
    unsigned acq = data->acqCount();
    if ((acq < _lastMatchedAcqCount) || (evrFiducials < _lastMatchedFiducial)) {
      noWrap = false;
      reset(false);
      if (server->debug() & 0x80) printf("Cspad2x2L1Action::fire resetting evrFiducials(0x%x) acq(0x%x)\n", evrFiducials, acq);
    }
    if (evrFiducials == _lastMatchedFiducial) printf("Cspad2x2L1Action::fire(in) saw duplicated evr fiducials 0x%x\n", evrFiducials); /* This is an EVR error, so what to do? */
    if ((server->debug() & 0x10000) == 0) {
      if (noWrap && ((evrFiducials-_lastMatchedFiducial) != (server->runTrigFactor() * 3 * (acq-_lastMatchedAcqCount)))) {
        frameError |= 1;
        if (_frameSyncErrorCount < FiducialErrorCountLimit) {
          printf("Cspad2x2L1Action::fire(in) frame mismatch!  evr(0x%x) lastMatchedFiducial(0x%x)\n\tframeNumber(0x%x), lastMatchedFrameNumber(0x%x), ",
              evrFiducials, _lastMatchedFiducial, data->frameNumber(), _lastMatchedFrameNumber);
          printf("acqCount(0x%x), lastMatchedAcqCount(0x%x)\n", acq, _lastMatchedAcqCount);
        }
      }
    }
    _lastMatchedFiducial = evrFiducials;
    _lastMatchedAcqCount = acq;
    if (_frameSyncErrorCount) frameError |= 1;  // make sure we don't reset the synchronization test

    if (!frameError) {
      _lastMatchedFiducial = evrFiducials;
      _lastMatchedFrameNumber = data->frameNumber();
      _lastMatchedAcqCount = data->acqCount();
      _frameSyncErrorCount = 0;
    }
    if (server->debug() & 0x40) printf("L1 acq - frm# %d\n", data->acqCount() - data->frameNumber());
    if (frameError) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (frameError&0xf));
      if (_frameSyncErrorCount++ < FiducialErrorCountLimit) {
        printf("Cspad2x2L1Action setting user damage due to frame error in frame 0x%x, fiducials 0x%x\n", data->frameNumber(), evrFiducials);
        if (!_frameSyncErrorCount) server->printHisto(false);
      } else {
        if (_frameSyncErrorCount == FiducialErrorCountLimit) {
          printf("Cspad2x2L1Action::fire(in) is turning off frame error reporting until we see a match\n");
        }
      }
    }
    if (!frameError) {
      server->process();
    }
  }
  return in;
}

class Cspad2x2ConfigAction : public Action {

  public:
    Cspad2x2ConfigAction( Pds::Cspad2x2ConfigCache& cfg, Cspad2x2Server* server)
    : _cfg( cfg ), _server(server), _result(0), _occPool(new GenericPool(sizeof(UserMessage),4))
      {}

    ~Cspad2x2ConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("Cspad2x2ConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (CsPad2x2ConfigType*)_cfg.current()))
            && _result != 0xdead
            && count++<5) {
          printf("\nCspad2x2ConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_result == 0 && _server->debug() & 0x10) _cfg.printRO();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Cspad2x2ConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_result == 0 && _server->debug() & 0x10) _cfg.printRO();
      if( _result || strlen(_server->pgp()->errorString())) {
        printf( "*** Cspad2x2ConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Cspad2x2  on host %s failed to configure!\n", getenv("HOSTNAME"));
        UserMessage* umsg = new (_occPool) UserMessage;
        umsg->append(message);
        umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server->xtc().src)));
        umsg->append("\n");
        umsg->append(_server->pgp()->errorString());
        _server->pgp()->clearErrorString();
        _server->manager()->appliance().post(umsg);
      }
      return in;
    }

  private:
    Cspad2x2ConfigCache&   _cfg;
    Cspad2x2Server*     _server;
    unsigned            _result;
    GenericPool*        _occPool;
};

class Cspad2x2BeginCalibCycleAction : public Action {
  public:
    Cspad2x2BeginCalibCycleAction(Cspad2x2Server* s, Cspad2x2ConfigCache& cfg, Cspad2x2L1Action& l1) : _server(s),
    _cfg(cfg), _l1(l1), _result(0), _occPool(new GenericPool(sizeof(UserMessage),4)) {};

    Transition* fire(Transition* tr) {
      printf("Cspad2x2BeginCalibCycleAction:;fire(Transition) payload size %u ", _server->payloadSize());
      _l1.reset();
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (CsPad2x2ConfigType*)_cfg.current())) && count++<5) {
            printf("\nCspad2x2BeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printRO();
        }
      }
      printf("enabled\n");
     if(_server->enable()) {
       _result = 0xcf;
     }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Cspad2x2BeginCalibCycleAction:;fire(InDatagram) payload size %u ", _server->payloadSize());
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printRO();
      } else printf("\n");
      if( _result || strlen(_server->pgp()->errorString()) ) {
        printf( "*** Cspad2x2ConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Cspad2x2  on host %s failed to configure!\n", getenv("HOSTNAME"));
        UserMessage* umsg = new (_occPool) UserMessage;
        umsg->append(message);
        umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server->xtc().src)));
        umsg->append("\n");
        umsg->append(_server->pgp()->errorString());
        _server->pgp()->clearErrorString();
        _server->manager()->appliance().post(umsg);
      }
      return in;
    }
  private:
    Cspad2x2Server*      _server;
    Cspad2x2ConfigCache& _cfg;
    Cspad2x2L1Action&    _l1;
    unsigned          _result;
    GenericPool*        _occPool;
};


class Cspad2x2UnconfigAction : public Action {
 public:
   Cspad2x2UnconfigAction( Cspad2x2Server* server, Cspad2x2ConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~Cspad2x2UnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("Cspad2x2UnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     if (!(_server->debug() & 0x2000)) _server->dumpFrontEnd();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("Cspad2x2UnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d cspad2x2 Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   Cspad2x2Server*      _server;
   Cspad2x2ConfigCache& _cfg;
   unsigned          _result;
};

class Cspad2x2EndCalibCycleAction : public Action {
  public:
    Cspad2x2EndCalibCycleAction(Cspad2x2Server* s, Cspad2x2ConfigCache& cfg) : _server(s), _cfg(cfg), _result(0), _bits(0) {};

    Transition* fire(Transition* tr) {
      printf("Cspad2x2EndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      unsigned result = _server->unconfigure();
      result |= _server->disable();
      if (_server->debug() & 0x2000) _server->dumpFrontEnd();
      _server->printHisto(true);
      if (result) _result = 0xcf;
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("Cspad2x2EndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** Cspad2x2EndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    Cspad2x2Server*      _server;
    Cspad2x2ConfigCache& _cfg;
    unsigned          _result;
    unsigned          _bits;
};

Cspad2x2Manager::Cspad2x2Manager( Cspad2x2Server* server) :
    _fsm(*new Fsm), _cfg(*new Cspad2x2ConfigCache(server->client())) {

   printf("Cspad2x2Manager being initialized... " );


   server->manager(this);

   Cspad2x2L1Action* l1 = new Cspad2x2L1Action( server );
   _fsm.callback( TransitionId::L1Accept, l1 );

   _fsm.callback( TransitionId::Map, new Cspad2x2AllocAction( _cfg ) );
   _fsm.callback( TransitionId::Unmap, new Cspad2x2UnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new Cspad2x2ConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new Cspad2x2EnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new Cspad2x2DisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new Cspad2x2BeginCalibCycleAction( server, _cfg, *l1 ) );
   _fsm.callback( TransitionId::EndCalibCycle, new Cspad2x2EndCalibCycleAction( server, _cfg ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new Cspad2x2EndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new Cspad2x2UnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new Cspad2x2BeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new Cspad2x2EndRunAction( server ) );
}
