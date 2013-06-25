/*
 * pnCCDManager.cc
 *
 *  Created on: May 30, 2013
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
#include "pds/config/pnCCDConfigType.hh"
#include "pds/pnccd/pnCCDServer.hh"
#include "pds/pnccd/FrameV0.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/pnccd/pnCCDManager.hh"
#include "pds/pnccd/pnCCDServer.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"
#include "pds/config/pnCCDConfigType.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class InDatagram;

  class pnCCDConfigCache : public CfgCache {
    public:
      enum {NumberOfConfigurations=1};
      pnCCDConfigCache(const Src& src) :
        CfgCache(src, _pnCCDConfigType, NumberOfConfigurations * sizeof(pnCCDConfigType)) {
        printf("pnCCDConfigCache contuctor space %u bytes\n",
            NumberOfConfigurations * sizeof(pnCCDConfigType));
      }
    public:
     void printCurrent() {
       pnCCDConfigType* cfg = (pnCCDConfigType*)current();
       printf("pnCCDConfigCache::printCurrent current 0x%x\n", (unsigned) (unsigned long)cfg);
     }
     void* allocate() {
       _cur_config = _buffer;
       _end_config = _buffer + _configtc.sizeofPayload();
       _changed = true;
       _configtc.damage = 0;
       _configtc.extent = sizeof(Xtc) + _size(_cur_config);
       printf("pnCCDConfigCache::allocate current %p %p %u\n", _buffer, _cur_config, _configtc.sizeofPayload());
       return _buffer;
     }
    private:
      int _size(void* tc) const { return sizeof(pnCCDConfigType); }
  };
}


using namespace Pds;

class pnCCDAllocAction : public Action {

  public:
   pnCCDAllocAction(pnCCDConfigCache& cfg, pnCCDServer* svr)
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
   pnCCDConfigCache& _cfg;
   pnCCDServer*      _svr;
};

class pnCCDUnmapAction : public Action {
  public:
    pnCCDUnmapAction(pnCCDServer* s) : server(s) {};

    Transition* fire(Transition* tr) {
      printf("pnCCDUnmapAction::fire(tr) disabling front end\n");
      server->disable();
      return tr;
    }

  private:
    pnCCDServer* server;
};

class pnCCDL1Action : public Action {
 public:
   pnCCDL1Action(pnCCDServer* svr);

   InDatagram* fire(InDatagram* in);

   pnCCDServer* server;
};

pnCCDL1Action::pnCCDL1Action(pnCCDServer* svr) : server(svr) {}

InDatagram* pnCCDL1Action::fire(InDatagram* in) {
  if (server->debug() & 8) printf("pnCCDL1Action::fire!\n");
  if (in->datagram().xtc.damage.value() == 0) {
    Datagram& dg = in->datagram();
    Xtc* xtc = &(dg.xtc);
    if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
      xtc = (Xtc*) xtc->payload();
      if (xtc->contains.id() != Pds::TypeId::Id_pnCCDframe) {
        printf("pnCCDL1Action::fire inner xtc not Id_pnCCDData, but %s!\n",
            xtc->contains.name(xtc->contains.id()));
        return in;
      }
    } else {
      printf("pnCCDL1Action::fire outer xtc not Id_Xtc, but %s!\n",
          xtc->contains.name(xtc->contains.id()));
      return in;
    }
//      server->process();
  }
  return in;
}

class pnCCDConfigAction : public Action {

  public:
    pnCCDConfigAction( Pds::pnCCDConfigCache& cfg, pnCCDServer* server, std::string sConfigFile)
    : _cfg( cfg ), _server(server), _result(0), _occPool(new GenericPool(sizeof(UserMessage),4)),
      _sConfigFile(sConfigFile) {
        strcpy(_errorMsg, "");
      }

    ~pnCCDConfigAction() {}

    Transition* fire(Transition* tr) {
      _result = 0;
      try {
        new(_cfg.allocate()) pnCCDConfigType(_sConfigFile);
      } catch (std::string& error) {
        strcpy(_errorMsg, error.c_str());
        _result = 42;
      }
      if (_result == 0) {
        printf("New pnCCD config from binary file\n");
        const pnCCDConfigType* config = (const pnCCDConfigType*)_cfg.current();
        printf("\tnumLinks(%u) payloadPerLink(%u)\n", config->numLinks(), config->payloadSizePerLink());
        _result = _server->configure((pnCCDConfigType*)_cfg.current());
      } else printf("Config error: %s\n", _errorMsg );
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("pnCCDConfigAction::fire(InDatagram) recorded\n");
      _cfg.record(in);
      if (_server->debug() & 0x10) _cfg.printCurrent();
      if( _result ) {
        printf( "*** pnCCDConfigAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
        char message[400];
        sprintf(message, "pnCCD  on host %s failed to configure!\n%s", getenv("HOSTNAME"),
            _result == 42 ? _errorMsg : "");
        UserMessage* umsg = new (_occPool) UserMessage;
        umsg->append(message);
        if (_result == 42) {
          umsg->append(_sConfigFile.c_str());
          umsg->append("\n");
        }
        umsg->append(DetInfo::name(static_cast<const DetInfo&>(_server->xtc().src)));
        _server->manager()->appliance().post(umsg);
      }
      return in;
    }

  private:
    pnCCDConfigCache&   _cfg;
    pnCCDServer*        _server;
  unsigned              _result;
  GenericPool*          _occPool;
  std::string           _sConfigFile;
  char                  _errorMsg[200];
};

class pnCCDBeginCalibCycleAction : public Action {
  public:
    pnCCDBeginCalibCycleAction(pnCCDServer* s, pnCCDConfigCache& cfg) : _server(s), _cfg(cfg) {};

    Transition* fire(Transition* tr) {
      printf("pnCCDBeginCalibCycleAction:;fire(Transition) enabled\n");
      _server->enable();
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("pnCCDBeginCalibCycleAction:;fire(InDatagram)\n");
      return in;
    }
  private:
    pnCCDServer*      _server;
    pnCCDConfigCache& _cfg;
};


class pnCCDUnconfigAction : public Action {
 public:
   pnCCDUnconfigAction( pnCCDServer* server, pnCCDConfigCache& cfg ) : _server( server ), _cfg(cfg) {}
   ~pnCCDUnconfigAction() {}

   Transition* fire(Transition* tr) {
     printf("pnCCDUnconfigAction:;fire(Transition) unconfigured\n");
     _result = _server->unconfigure();
     return tr;
   }

   InDatagram* fire(InDatagram* in) {
     printf("pnCCDUnconfigAction:;fire(InDatagram)\n");
      if( _result ) {
         printf( "*** Found %d pnccd Unconfig errors\n", _result );
         in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
         in->datagram().xtc.damage.userBits(0xda);
      }
      return in;
   }

 private:
   pnCCDServer*      _server;
   pnCCDConfigCache& _cfg;
   unsigned          _result;
};

class pnCCDEndCalibCycleAction : public Action {
  public:
    pnCCDEndCalibCycleAction(pnCCDServer* s, pnCCDConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

    Transition* fire(Transition* tr) {
      printf("pnCCDEndCalibCycleAction:;fire(Transition) %p", _cfg.current());
      _cfg.next();
      printf(" %p\n", _cfg.current());
      _server->disable();
      _result = _server->unconfigure();
      _server->dumpFrontEnd();
      _server->printHisto(true);
      return tr;
    }

    InDatagram* fire(InDatagram* in) {
      printf("pnCCDEndCalibCycleAction:;fire(InDatagram)\n");
      if( _result ) {
        printf( "*** pnCCDEndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
        if (in->datagram().xtc.damage.value() == 0) {
          in->datagram().xtc.damage.increase(Damage::UserDefined);
          in->datagram().xtc.damage.userBits(_result);
        }
      }
      return in;
    }

  private:
    pnCCDServer*      _server;
    pnCCDConfigCache& _cfg;
    unsigned          _result;
};

pnCCDManager::pnCCDManager( pnCCDServer* server, unsigned d, std::string& sConfigFile) :
    _fsm(*new Fsm), _cfg(*new pnCCDConfigCache(server->client())),
    _occPool(new GenericPool(sizeof(UserMessage),4)), _sConfigFile(sConfigFile) {

   printf("pnCCDManager being initialized... " );

   unsigned ports = (d >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%u", d);
   } else {
     sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
   }

   int pnccd = open( devName,  O_RDWR | O_NONBLOCK);
   printf("pgpcard file number %d\n", pnccd);
   if (pnccd < 0) {
     sprintf(err, "pnCCDManager::pnCCDManager() opening %s failed", devName);
     perror(err);
     // What else to do if the open fails?
     char message[400];
     sprintf(message, "pnCCD segment level could not open the pgpcard %s on %s\n", devName, getenv("HOSTNAME"));
     UserMessage* umsg = new (_occPool) UserMessage;
     umsg->append(message);
     this->appliance().post(umsg);
     sleep(1);
     ::exit(-1);
   }

   unsigned offset = 0;
   while ((((ports>>offset) & 1) == 0) && (offset < 5)) {
     offset += 1;
   }

   Pgp::Pgp::portOffset(offset);

   if (offset >= 4) {
     printf("pnCCDManager::pnCCDManager() illegal port mask!! 0x%x\n", ports);
   }

   server->manager(this);
   server->setpnCCD( pnccd );
//   server->laneTest();

   _fsm.callback( TransitionId::Map, new pnCCDAllocAction( _cfg, server ) );
   _fsm.callback( TransitionId::Unmap, new pnCCDUnmapAction( server ) );
   _fsm.callback( TransitionId::Configure, new pnCCDConfigAction(_cfg, server, sConfigFile ) );
   //   _fsm.callback( TransitionId::Enable, new pnCCDEnableAction( server ) );
   //   _fsm.callback( TransitionId::Disable, new pnCCDDisableAction( server ) );
   _fsm.callback( TransitionId::BeginCalibCycle, new pnCCDBeginCalibCycleAction( server, _cfg ) );
   _fsm.callback( TransitionId::EndCalibCycle, new pnCCDEndCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::L1Accept, new pnCCDL1Action( server ) );
   //   _fsm.callback( TransitionId::EndCalibCycle, new pnCCDEndCalibCycleAction( server ) );
   _fsm.callback( TransitionId::Unconfigure, new pnCCDUnconfigAction( server, _cfg ) );
   // _fsm.callback( TransitionId::BeginRun,
   //                new pnCCDBeginRunAction( server ) );
   // _fsm.callback( TransitionId::EndRun,
   //                new pnCCDEndRunAction( server ) );
}
