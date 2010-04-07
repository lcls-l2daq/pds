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
      //      unint64_t 
      //      unsigned long long c0 = _server[0]->GetTriggerCounter1();
      //      unsigned long long c1 = _server[1]->GetTriggerCounter1();
      //      printf("trig count 0: 0x%llx, trig count 1: 0x%llx\n", c0, c1);
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
  enum {MaxConfigSize=0x100000};
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
    // todo: get configuration from database.
    // program/validate register settings
    //    int len = _cfg.fetch(*tr,_ipimbConfigType, &_config, sizeof(_config));
    int len = 1;
    if (len <= 0) {
      printf("IpimbConfigAction: failed to retrieve configuration : (%d)%s.  Applying default.\n",errno,strerror(errno));
    }
    //    new (&_config) Ipimb::ConfigV1(0, 1, 2, 3, 4.1, 0.5, 6, 7, 0, 8, 9);
    IpimbConfigType _config[_nServers];
    _nDamagedConfigures = 0;
    for (unsigned i=0; i<_nServers; i++) {
      int len = (*_cfg[i]).fetch(*tr,_ipimbConfigType, &_config[i], sizeof(_config[i]));
      printf("config length returned: %d\n", len);
      //_config[i] = *new Ipimb::ConfigV1(2, 3, 4, 5, 4.1, 0., 0.5, 8, 9*i);
      _config[i].dump();
      if (!_server[i]->configure(_config[i])) {
	printf("Ipimb server %d configuration was damaged\n", i);
	_nDamagedConfigures += 1;
      }
      _server[i]->setFakeCount(_server[i]->GetTriggerCounter1());
    }
    //    printf("fake success back from ipim board class: %d\n", _nerror);
    return tr;
  }

private:
  CfgClientNfs** _cfg;
  //  IpimBoard* _ipimBoard;
  //  IpimbServer& _server;
  IpimbServer** _server;
  unsigned _nServers;
  unsigned _nDamagedConfigures;
  IpimbConfigType* _config;
};

Appliance& IpimbManager::appliance() {return _fsm;}

//IpimbManager::IpimbManager(IpimbServer& server, unsigned nServer, CfgClientNfs& cfg): // had
IpimbManager::IpimbManager(IpimbServer* server[], unsigned nServers, CfgClientNfs** cfg) :
  _fsm(*new Fsm), _nServers(nServers) {
  //  printf("Manager for ipimbs initialized with %d servers\n", _nServers);
  char portName[12];
  for (unsigned i=0; i<_nServers; i++) {
    sprintf(portName, "/dev/ttyPS%d", i);//*6+1);
    //    printf("will make new ipimb at port %s\n", portName);
    IpimBoard* ipimBoard = new IpimBoard(portName);
    //    printf("New IpimBoard fd is %d\n", ipimBoard->get_fd());
    server[i]->setIpimb(ipimBoard); // this is obviously wrong
  }
  Action* caction = new IpimbConfigAction(cfg, server, _nServers); // action needs server to free fd during configure
  _fsm.callback(TransitionId::Configure,caction);
  IpimbL1Action& ipimbl1 = *new IpimbL1Action();
  _fsm.callback(TransitionId::Map, new IpimbAllocAction(cfg, _nServers));
  _fsm.callback(TransitionId::L1Accept,&ipimbl1);
}
