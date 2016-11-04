/*
 * ImpManager.cc
 *
 *  Created on: April 12, 2013
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
#include "pds/config/ImpConfigType.hh"
#include "pds/imp/ImpServer.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/imp/ImpManager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;

  class ImpConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=1000};
      ImpConfigCache(const Src& src) :
        CfgCache(src, _ImpConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(ImpConfigType)) {}
    public:
     void printCurrent() {
       ImpConfigType* cfg = (ImpConfigType*)current();
       printf("ImpConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
    private:
      int _size(void* tc) const { return sizeof(ImpConfigType); }
  };
}


using namespace Pds;

class ImpAllocAction : public Action {

  public:
   ImpAllocAction(ImpConfigCache& cfg, ImpServer* svr)
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
   ImpConfigCache& _cfg;
   ImpServer*      _svr;
};

class ImpUnmapAction : public Action {
  public:
    ImpUnmapAction(ImpServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("ImpUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    ImpServer* server;
};

class ImpL1Action : public Action {
 public:
   ImpL1Action(ImpServer* svr);

   InDatagram* fire(InDatagram* in);

   ImpServer* server;
   unsigned _lastMatchedFiducial;
   unsigned _ioIndex;
   bool     _fiducialError;

};

ImpL1Action::ImpL1Action(ImpServer* svr) :
    server(svr),
    _lastMatchedFiducial(0xfffffff),
    _fiducialError(false) {}

InDatagram* ImpL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("ImpL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
//    Pds::Pgp::DataImportFrame* data;
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
//    unsigned evrFiducials = dg.seq.stamp().fiducials();
    unsigned error = 0;
    char*    payload;
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() == Pds::TypeId::Id_ImpData) {
        payload = xtc->payload();
      } else {
        printf("ImpL1Action::fire inner xtc not Id_ImpData, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("ImpL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }

    if (error) {
      dg.xtc.damage.increase(Pds::Damage::UserDefined);
      dg.xtc.damage.userBits(0xf0 | (error&0xf));
      printf("ImpL1Action setting user damage due to fiducial in quads(0x%x)\n", error);
      if (!_fiducialError) server->printHisto(false);
      else _fiducialError = true;
    } else {
//      server->process();
    }
  }
  return in;
}

class ImpConfigAction : public Action {

  public:
    ImpConfigAction( Pds::ImpConfigCache& cfg, ImpServer* server)
    : _cfg( cfg ), _server(server), _result(0), _occPool(new GenericPool(sizeof(UserMessage),4))

      {}

    ~ImpConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      int i = _cfg.fetch(tr);
      printf("ImpConfigAction::fire(Transition) fetched %d\n", i);
      _server->resetOffset();
      if (_cfg.scanning() == false) {
        unsigned count = 0;
        while ((_result = _server->configure( (ImpConfigType*)_cfg.current())) && count++<3) {
          printf("\nImpConfigAction::fire(tr) retrying config %u\n", count);
        };
        if (_server->debug() & 0x10) _cfg.printCurrent();
      }
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("ImpConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result || strlen(_server->pgp()->errorString()) ) {
        printf( "*** ImpConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Imp  on host %s failed to configure!\n", getenv("HOSTNAME"));
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
    ImpConfigCache&              _cfg;
    ImpServer*                   _server;
  unsigned                       _result;
  GenericPool*                   _occPool;
};

class ImpBeginCalibCycleAction : public Action {
  public:
    ImpBeginCalibCycleAction(ImpServer* s, ImpConfigCache& cfg) : _server(s),
    _cfg(cfg), _result(0), _occPool(new GenericPool(sizeof(UserMessage),4)) {};

    Transition* fire(Transition* tr) {
      printf("ImpBeginCalibCycleAction:;fire(Transition) ");
      if (_cfg.scanning()) {
        if (_cfg.changed()) {
          printf("configured and \n");
//          _server->offset(_server->offset()+_server->myCount()+1);
          unsigned count = 0;
          while ((_result = _server->configure( (ImpConfigType*)_cfg.current())) && count++<3) {
            printf("\nImpBeginCalibCycleAction::fire(tr) retrying config %u\n", count);
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        }
      }
      printf("enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("ImpBeginCalibCycleAction:;fire(InDatagram)");
      if (_cfg.scanning() && _cfg.changed()) {
        printf(" recorded\n");
        _cfg.record(in);
        if (_server->debug() & 0x10) _cfg.printCurrent();
      } else printf("\n");
      if( _result || strlen(_server->pgp()->errorString()) ) {
        printf( "*** ImpConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "Imp  on host %s failed to configure!\n", getenv("HOSTNAME"));
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
    ImpServer*                     _server;
    ImpConfigCache&                _cfg;
    unsigned                       _result;
    GenericPool*                   _occPool;
};


class ImpUnconfigAction : public Action {
 public:
   ImpUnconfigAction( ImpServer* server, ImpConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~ImpUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("ImpUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("ImpUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d imp Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   ImpServer*      _server;
   ImpConfigCache& _cfg;
   unsigned          _result;
};

class ImpEndCalibCycleAction : public Action {
  public:
    ImpEndCalibCycleAction(ImpServer* s, ImpConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("ImpEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _server->disable();
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("ImpEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** ImpEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    ImpServer*      _server;
    ImpConfigCache& _cfg;
    unsigned          _result;
};

ImpManager::ImpManager( ImpServer* server, int d) :
    _fsm(*new Fsm), _cfg(*new ImpConfigCache(server->client())) {

   printf("ImpManager being initialized... " );

   server->manager(this);

   _fsm.callback( TransitionId::Map, new ImpAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new ImpUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new ImpConfigAction(_cfg, server ) );
   //   _fsm.callback( TransitionId::Enable, new ImpEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new ImpDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new ImpBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new ImpEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new ImpL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new ImpEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new ImpUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new ImpBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new ImpEndRunAction( server ) );
}
