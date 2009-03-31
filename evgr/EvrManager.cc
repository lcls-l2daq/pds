
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <new>
#include <pthread.h>
#include <semaphore.h>

#include "evgr/evr/evr.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/Route.hh"
#include "pds/service/Client.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/config/EvrConfigType.hh"
#include "EvgrBoardInfo.hh"
#include "EvrManager.hh"
#include "EvgrOpcode.hh"

using namespace Pds;

class L1Xmitter;
static L1Xmitter* l1xmitGlobal;
static EvgrBoardInfo<Evr> *erInfoGlobal;  // yuck

class L1Xmitter {
public:
  L1Xmitter(Evr& er) :
    _er(er),
    _outlet(sizeof(EvrDatagram),0,
	    Ins(Route::interface())),
    _evtCounter(0), _enabled(false) {}
  void xmit() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ClockTime ctime(ts.tv_sec,ts.tv_nsec);
    Sequence seq(Sequence::Event,TransitionId::L1Accept,ctime,
                 fe.TimestampLow,fe.TimestampHigh);
    EvrDatagram datagram(seq, _evtCounter++);
    _outlet.send((char*)&datagram,0,0,_dst);
//     printf("Received opcode 0x%x timestamp 0x%x/0x%x count %d\n",
//            fe.EventCode,fe.TimestampHigh,fe.TimestampLow,_evtCounter-1);
  }
  void reset() { _evtCounter = 0; }
  void enable(bool e) { _enabled=e; }
  bool enable() const { return _enabled; }
  void dst(const Ins& ins) { _dst=ins; }
private:
  Evr& _er;
  Client   _outlet;
  Ins      _dst;
  unsigned _evtCounter;
  bool     _enabled;
};

class EvrAction : public Action {
protected:
  EvrAction(Evr& er) : _er(er) {}
  Evr& _er;
};

class EvrL1Action : public EvrAction {
public:
  EvrL1Action(Evr& er) : EvrAction(er) {}
  InDatagram* fire(InDatagram* in) {
    return in;
  }
};

class EvrEnableAction : public EvrAction {
public:
  EvrEnableAction(Evr& er) :
    EvrAction(er) {}
  Transition* fire(Transition* tr) {
    unsigned ram=0;
    _er.MapRamEnable(ram,1);
    return tr;
  }
};

class EvrDisableAction : public EvrAction {
public:
  EvrDisableAction(Evr& er) : EvrAction(er) {}
  Transition* fire(Transition* tr) {
    unsigned ram=0;
    _er.MapRamEnable(ram,0);
    return tr;
  }
};

class EvrBeginRunAction : public EvrAction {
public:
  EvrBeginRunAction(Evr& er) : EvrAction(er) {}
  Transition* fire(Transition* tr) {
    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    _er.EnableFIFO(1);
    _er.Enable(1);
    return tr;
  }
};

class EvrEndRunAction : public EvrAction {
public:
  EvrEndRunAction(Evr& er) : EvrAction(er) {}
  Transition* fire(Transition* tr) {
    _er.IrqEnable(0);
    _er.Enable(0);
    _er.EnableFIFO(0);
    return tr;
  }
};

class EvrConfigAction : public EvrAction {
public:
  EvrConfigAction(Evr& er,
		  EvgrOpcode::Opcode opcode,
		  CfgClientNfs& cfg) :
    EvrAction(er),
    _opcode(opcode),
    _cfg(cfg),
    _cfgtc(_evrConfigType,cfg.src()),
    _configBuffer(new char[sizeof(EvrConfigType)+
			   32*sizeof(EvrData::PulseConfig)+
			   10*sizeof(EvrData::OutputMap)])
  {
  }

  ~EvrConfigAction()
  { delete[] _configBuffer; }

  InDatagram* fire(InDatagram* dg) {
    dg->insert(_cfgtc, _configBuffer);
    return dg;
  }

  Transition* fire(Transition* tr) {
    _cfgtc.damage.increase(Damage::UserDefined);
    int len=_cfg.fetch(*tr,_evrConfigType, _configBuffer);
    if (len<0) {
      printf("Config::configure failed to retrieve Evr configuration\n");
      return tr;
    }
    const EvrConfigType& cfg = *new(_configBuffer) EvrConfigType;
    _cfgtc.extent = sizeof(Xtc)+cfg.size();

    printf("Configuring evr\n");

    _er.Reset();

    // setup map ram
    int ram=0; int enable=l1xmitGlobal->enable();
    _er.MapRamEnable(ram,0);
    _er.SetFIFOEvent(ram, _opcode, enable);

    for(unsigned k=0; k<cfg.npulses(); k++) {
      const EvrData::PulseConfig& pc = cfg.pulse(k);
      _er.SetPulseMap(ram, _opcode, pc.trigger(), pc.set(), pc.clear());
      _er.SetPulseProperties(pc.pulse(), 
			     pc.polarity() ? 0 : 1, 
			     pc.map_reset_enable(),
			     pc.map_set_enable(),
			     pc.map_trigger_enable(),
			     1);
      _er.SetPulseParams(pc.pulse(), pc.prescale(), 
			 pc.delay(), pc.width());
    }

    for(unsigned k=0; k<cfg.noutputs(); k++) {
      const EvrData::OutputMap& map = cfg.output_map(k);
      switch(map.conn()) {
      case EvrData::OutputMap::FrontPanel: 
	_er.SetFPOutMap( map.conn_id(), map.map()); 
	break;
      case EvrData::OutputMap::UnivIO:
	_er.SetUnivOutMap( map.conn_id(), map.map());
	break;
      }
    }

    l1xmitGlobal->reset();

    _cfgtc.damage = 0;
    return tr;
  }

private:
  EvgrOpcode::Opcode _opcode;
  CfgClientNfs& _cfg;
  Xtc _cfgtc;
  char* _configBuffer;
};

class EvrAllocAction : public Action {
public:
  EvrAllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
  Transition* fire(Transition* tr) {
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    _cfg.initialize(alloc.allocation());
    //
    //  Test if we own the primary EVR for this partition
    //
#if 0
    unsigned    nnodes = alloc.nnodes();
    int pid = getpid();
    for (unsigned n=0; n<nnodes; n++) {
      const Node* node = alloc.node(n);
      if (node->pid() == pid) {
	printf("Found our EVR\n");
	l1xmitGlobal->enable(true);
	break;
      }
      else if (node->level() == Level::Segment) {
	printf("Found other EVR\n");
	l1xmitGlobal->enable(false);
	break;
      }
    }
#else
    l1xmitGlobal->enable(true);
#endif
    l1xmitGlobal->dst(StreamPorts::event(alloc.allocation().partitionid(),
					 Level::Segment));
    return tr;
  }
private:
  CfgClientNfs& _cfg;
};

extern "C" {
  void evrmgr_sig_handler(int parm)
  {
    Evr& er = erInfoGlobal->board();
    int flags = er.GetIrqFlags();
    if (flags & EVR_IRQFLAG_EVENT)
      {
        er.ClearIrqFlags(EVR_IRQFLAG_EVENT);
        l1xmitGlobal->xmit();
      }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance& EvrManager::appliance() {return _fsm;}

EvrManager::EvrManager(EvgrBoardInfo<Evr> &erInfo, 
		       CfgClientNfs & cfg,
                       EvgrOpcode::Opcode opcode) :
  _er(erInfo.board()),_fsm(*new Fsm) {

  l1xmitGlobal = new L1Xmitter(_er);
  _fsm.callback(TransitionId::Map, new EvrAllocAction(cfg));
  _fsm.callback(TransitionId::Configure,new EvrConfigAction(_er,opcode,cfg));
  _fsm.callback(TransitionId::BeginRun,new EvrBeginRunAction(_er));
  _fsm.callback(TransitionId::Enable,new EvrEnableAction(_er));
  _fsm.callback(TransitionId::EndRun,new EvrEndRunAction(_er));
  _fsm.callback(TransitionId::Disable,new EvrDisableAction(_er));
  _fsm.callback(TransitionId::L1Accept,new EvrL1Action(_er));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}
