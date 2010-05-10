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
#include "pds/config/IpimbConfigType.hh"
#include "IpimBoard.hh"
#include "IpimbManager.hh"
#include "IpimbServer.hh"
#include "pdsdata/ipimb/DataV1.hh"
#include "pdsdata/ipimb/ConfigV1.hh"
#include "pds/config/CfgClientNfs.hh"

using namespace Pds;

class IpimbAction : public Action {
protected:
  IpimbAction() {}
};

class IpimbAllocAction : public Action {
public:
  IpimbAllocAction(CfgClientNfs** cfg, unsigned nServers) : _cfg(cfg), _nServers(nServers) {}
  Transition* fire(Transition* tr) {
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    for (unsigned i=0; i<_nServers; i++) {
      _cfg[i]->initialize(alloc.allocation());
    }
    return tr;
  }
private:
  CfgClientNfs** _cfg;
  unsigned _nServers;
};

class IpimbL1Action : public Action {
public:
  IpimbL1Action() {}
  InDatagram* fire(InDatagram* in) {
    Datagram& dg = in->datagram();
    // todo: may want to validate the datagram here?  but maybe not.
    // if (!dg.xtc.damage.value()) validate(in);
    //dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    if (int d = dg.xtc.damage.value()) {
      printf("Ipimb damage 0x%x\n",d);
    }
    //    printf("L1 called back with payload ptr %p\n", dg.xtc.payload());
    //    unsigned* data = (unsigned*)(dg.xtc.payload());
    //    for (unsigned i=0;i<48;i++) {
    //      printf("0x%8.8x ",data[i]);
    //      if ((i+1)%8==0) printf("\n");
    //  }
    return in;
  }
};

class IpimbConfigAction : public IpimbAction {
public:
  IpimbConfigAction(CfgClientNfs** cfg, IpimbServer* server[], int nServers) :
    _cfg(cfg), _server(server), _nServers(nServers), _nDamagedConfigures(0) {}
  ~IpimbConfigAction() {}
  InDatagram* fire(InDatagram* dg) {
    //    printf("in ipimb config indatagram transition\n");
    // todo: report the results of configuration to the datastream.
    // insert assumes we have enough space in the input datagram
    for (unsigned i=0; i<_nServers; i++) {
      Xtc _cfgtc = Xtc(_ipimbConfigType, _server[i]->client());
      dg->insert(_cfgtc, &_config[i]);
    }
    if (_nDamagedConfigures) {
      printf("*** Found %d ipimb configuration errors\n",_nDamagedConfigures);
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }
  Transition* fire(Transition* tr) {
    IpimbConfigType _config[_nServers];
    _nDamagedConfigures = 0;
    for (unsigned i=0; i<_nServers; i++) {
      int len = (*_cfg[i]).fetch(*tr,_ipimbConfigType, &_config[i], sizeof(_config[i]));
      if (len <= 0) {
	printf("IpimbConfigAction: failed to retrieve configuration : (%d)%s.  Refusing to configure.\n",errno,strerror(errno));
	_nDamagedConfigures += 1;
	continue;
      }
      _config[i].dump();
      if (!_server[i]->configure(_config[i])) {
	printf("Ipimb server %d configuration was damaged\n", i);
	_nDamagedConfigures += 1;
      }
    }
    //    printf("fake success back from ipim board class: %d\n", _nerror);
    return tr;
  }

private:
  CfgClientNfs** _cfg;
  IpimbServer** _server;
  unsigned _nServers;
  unsigned _nDamagedConfigures;
  IpimbConfigType* _config;
};

class IpimbUnconfigAction : public IpimbAction {
public:
  IpimbUnconfigAction(IpimbServer* server[], int nServers) :
    _server(server), _nServers(nServers) {}
  ~IpimbUnconfigAction() {}
  Transition* fire(Transition* tr) {
    for (unsigned i=0; i<_nServers; i++) {
      if (!_server[i]->unconfigure()) {
	printf("Ipimb server %d unconfiguration complained\n", i);
      }
    }
    return tr;
  }

private:
  IpimbServer** _server;
  unsigned _nServers;
};

Appliance& IpimbManager::appliance() {return _fsm;}

IpimbManager::IpimbManager(IpimbServer* server[], unsigned nServers, CfgClientNfs** cfg) :
  _fsm(*new Fsm), _nServers(nServers) {
  char portName[12];
  for (unsigned i=0; i<_nServers; i++) {
    sprintf(portName, "/dev/ttyPS%d", i);//*6+1);
    IpimBoard* ipimBoard = new IpimBoard(portName);
    server[i]->setIpimb(ipimBoard); // this is obviously wrong
  }
  Action* caction = new IpimbConfigAction(cfg, server, _nServers);
  _fsm.callback(TransitionId::Configure, caction);
  Action* uncaction = new IpimbUnconfigAction(server, _nServers);
  _fsm.callback(TransitionId::Unconfigure, uncaction);
  IpimbL1Action& ipimbl1 = *new IpimbL1Action();
  _fsm.callback(TransitionId::Map, new IpimbAllocAction(cfg, _nServers));
  _fsm.callback(TransitionId::L1Accept,&ipimbl1);
}
