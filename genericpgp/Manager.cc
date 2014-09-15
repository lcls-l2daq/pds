/*
 * Manager.cc
 *
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
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerDataType.hh"
#include "pds/genericpgp/Server.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pdsdata/xtc/DetInfo.hh"
//#include "pdsdata/psddl/epix.ddl.h"
#include "pds/genericpgp/Manager.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pds/config/CfgCache.hh"

namespace Pds {
  namespace GenericPgp {
    class ConfigCache : public CfgCache {
    public:
      enum {StaticALlocationNumberOfConfigurationsForScanning=100};
      ConfigCache(const Src& src) :
        CfgCache(src, _genericPgpConfigType, StaticALlocationNumberOfConfigurationsForScanning * 135616) {}
    public:
      void printCurrent() {
        printf("ConfigCache::printCurrent current %p\n", current());
      }
    private:
      int _size(void* tc) const { return ((GenericPgpConfigType*)tc)->_sizeof(); }
    };

    class AllocAction : public Action {
    public:
      AllocAction(ConfigCache& cfg, Server* svr)
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
      ConfigCache& _cfg;
      Server*      _svr;
    };

    class UnmapAction : public Action {
    public:
      UnmapAction(Server* s) : server(s) {};
      
      Transition* fire(Transition* tr) {
        printf("UnmapAction::fire(tr) disabling front end\n");
        server->disable();
        return tr;
      }

    private:
      Server* server;
    };

    class ConfigAction : public Action {

    public:
      ConfigAction( ConfigCache& cfg, Server* server)
        : _cfg( cfg ), _server(server), _result(0)
      {}

      ~ConfigAction() {}

      Transition* fire(Transition* tr) {
        _result = 0;
        int i = _cfg.fetch(tr);
        printf("ConfigAction::fire(Transition) fetched %d\n", i);
        _server->resetOffset();
        if (_cfg.scanning() == false) {
          if ((_result = _server->configure( (GenericPgpConfigType*)_cfg.current()))) {
            printf("\nConfigAction::fire(tr) failed configuration\n");
          };
          if (_server->debug() & 0x10) _cfg.printCurrent();
        } else {
          _server->offset(0);
        }
        return tr;
      }

      InDatagram* fire(InDatagram* in) {
        printf("ConfigAction::fire(InDatagram) recorded\n");

	_cfg.record(in);

	const GenericPgpConfigType& c = 
	  *reinterpret_cast<const GenericPgpConfigType*>(_cfg.current());
	for(unsigned i=1; i<c.number_of_streams(); i++) {
	  if (c.stream()[i].data_type()==_epixSamplerDataType.value()) {
	    Xtc xtc(_epixSamplerConfigType,
		    _server->client());
	    const EpixSamplerConfigType& s = 
	      *reinterpret_cast<const EpixSamplerConfigType*>(c.payload().data()+
							      c.stream()[i].config_offset());
	    xtc.extent += EpixSamplerConfigType::_sizeof();
	    in->insert(xtc, &s);
	  }
	  else {
	    printf("Manager failed to insert unknown configuration 0x%x\n",
		   c.stream()[i].data_type());
	  }
	}

        if (_server->debug() & 0x10) _cfg.printCurrent();
        if( _result ) {
          printf( "*** ConfigAction found configuration errors _result(0x%x)\n", _result );
          if (in->datagram().xtc.damage.value() == 0) {
            in->datagram().xtc.damage.increase(Damage::UserDefined);
            in->datagram().xtc.damage.userBits(_result);
          }
        }
        return in;
      }

    private:
      ConfigCache&  _cfg;
      Server*       _server;
      unsigned      _result;
    };

    class BeginCalibCycleAction : public Action {
    public:
      BeginCalibCycleAction(Server* s, ConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

      Transition* fire(Transition* tr) {
        printf("BeginCalibCycleAction:;fire(tr) ");
        if (_cfg.scanning()) {
          if (_cfg.changed()) {
            printf("configured and ");
            _server->offset(_server->offset()+_server->myCount()+1);
            printf(" offset %u count %u\n", _server->offset(), _server->myCount());
            if ((_result = _server->configure( (GenericPgpConfigType*)_cfg.current()))) {
              printf("\nGenericPgpBeginCalibCycleAction::fire(tr) failed config\n");
            };
            if (_server->debug() & 0x10) _cfg.printCurrent();
          }
        }
        printf("BeginCalibCycleAction:;fire(tr) enabled\n");
        _server->enable();
        return tr;
      }

      InDatagram* fire(InDatagram* in) {
        printf("BeginCalibCycleAction:;fire(InDatagram)");
        if (_cfg.scanning() && _cfg.changed()) {
          printf(" recorded\n");
          _cfg.record(in);
          if (_server->debug() & 0x10) _cfg.printCurrent();
        } else printf("\n");
        if( _result ) {
          printf( "*** ConfigAction found configuration errors _result(0x%x)\n", _result );
          if (in->datagram().xtc.damage.value() == 0) {
            in->datagram().xtc.damage.increase(Damage::UserDefined);
            in->datagram().xtc.damage.userBits(_result);
          }
        }
        return in;
      }
    private:
      Server*       _server;
      ConfigCache&  _cfg;
      unsigned          _result;
    };

    class UnconfigAction : public Action {
    public:
      UnconfigAction( Server* server, ConfigCache& cfg ) : _server( server ), _cfg(cfg), _result(0) {}
      ~UnconfigAction() {}
      
      Transition* fire(Transition* tr) {
        printf("UnconfigAction:;fire(Transition) unconfigured\n");
        _result = _server->unconfigure();
        return tr;
      }

      InDatagram* fire(InDatagram* in) {
        printf("UnconfigAction:;fire(InDatagram)\n");
        if( _result ) {
          printf( "*** Found %d epix Unconfig errors\n", _result );
          in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
          in->datagram().xtc.damage.userBits(0xda);
        }
        return in;
      }

    private:
      Server*      _server;
      ConfigCache& _cfg;
      unsigned     _result;
    };

    class EndCalibCycleAction : public Action {
    public:
      EndCalibCycleAction(Server* s, ConfigCache& cfg) : _server(s), _cfg(cfg), _result(0) {};

      Transition* fire(Transition* tr) {
        printf("EndCalibCycleAction:;fire(Transition) %p", _cfg.current());
        _cfg.next();
        printf(" %p\n", _cfg.current());
        _result = _server->unconfigure();
        _server->dumpFrontEnd();
        _server->printHisto(true);
        return tr;
      }
      
      InDatagram* fire(InDatagram* in) {
        printf("EndCalibCycleAction:;fire(InDatagram)\n");
        if( _result ) {
          printf( "*** EndCalibCycleAction found configuration errors _result(0x%x)\n", _result );
          if (in->datagram().xtc.damage.value() == 0) {
            in->datagram().xtc.damage.increase(Damage::UserDefined);
            in->datagram().xtc.damage.userBits(_result);
          }
        }
        return in;
      }
      
    private:
      Server*      _server;
      ConfigCache& _cfg;
      unsigned     _result;
    };

  };
};

using namespace Pds;
using namespace Pds::GenericPgp;

Manager::Manager( Server* server, unsigned d) :
  _fsm(*new Fsm), _cfg(*new ConfigCache(server->client())) {

  printf("Manager being initialized... " );

  unsigned ports = (d >> 4) & 0xf;
  char devName[128];
  char err[128];
  if (ports == 0) {
    ports = 15;
    sprintf(devName, "/dev/pgpcard%u", d);
  } else {
    sprintf(devName, "/dev/pgpcard_%u_%u", d & 0xf, ports);
  }

  int fd = open( devName,  O_RDWR | O_NONBLOCK);
  printf("pgpcard %s file number %d\n", devName, fd);
  if (fd < 0) {
    sprintf(err, "GenericPgpManager::GenericPgpManager() opening %s failed", devName);
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
    printf("GenericPgpManager::GenericPgpManager() illegal port mask!! 0x%x\n", ports);
  }

  server->appliance(_fsm);
  server->setFd( fd );
//   server->laneTest();

  _fsm.callback( TransitionId::Map, new AllocAction( _cfg, server ) );
  _fsm.callback( TransitionId::Unmap, new UnmapAction( server ) );
  _fsm.callback( TransitionId::Configure, new ConfigAction(_cfg, server ) );
  //   _fsm.callback( TransitionId::Enable, new EnableAction( server ) );
  //   _fsm.callback( TransitionId::Disable, new DisableAction( server ) );
  _fsm.callback( TransitionId::BeginCalibCycle, new BeginCalibCycleAction( server, _cfg ) );
  _fsm.callback( TransitionId::EndCalibCycle, new EndCalibCycleAction( server, _cfg ) );
  //  _fsm.callback( TransitionId::L1Accept, new L1Action( server ) );
  _fsm.callback( TransitionId::Unconfigure, new UnconfigAction( server, _cfg ) );
  // _fsm.callback( TransitionId::BeginRun,
  //                new BeginRunAction( server ) );
  // _fsm.callback( TransitionId::EndRun,
  //                new EndRunAction( server ) );
}
