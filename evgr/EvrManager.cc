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

#include "pds/service/Timer.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/EnableEnv.hh"

namespace Pds {
  class DoneTimer : public Timer {
  public:
    DoneTimer(Appliance& app) :
      _app    (app),
      _pool   (sizeof(Occurrence),1),
      _task   (new Task(TaskObject("donet"))) {}
    ~DoneTimer() { _task->destroy(); }
  public:
    void set_duration_ms(unsigned v) { _duration = v; }
  public:
    void     expired () { _app.post(new (&_pool) Occurrence(OccurrenceId::SequencerDone)); }
    Task*    task    () { return _task; }
    unsigned duration() const { return _duration; }
    unsigned repetitive() const { return 0; }
  private:
    Appliance&   _app;
    GenericPool  _pool;
    Task*        _task;
    unsigned     _duration;
  };
};

using namespace Pds;

static unsigned dropPulseMask = 0xffffffff;

class L1Xmitter;
static L1Xmitter* l1xmitGlobal;
static EvgrBoardInfo<Evr> *erInfoGlobal;  // yuck

class L1Xmitter {
public:
  L1Xmitter(Evr& er, DoneTimer& done) :
    _er    (er),
    _done  (done),
    _outlet(sizeof(EvrDatagram),0,
	    Ins(Route::interface())),
    _evtCounter(0), _evtStop(0), _enabled(false) {}
  void xmit() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ClockTime ctime(ts.tv_sec,ts.tv_nsec);
    TimeStamp stamp(fe.TimestampLow,fe.TimestampHigh, _evtCounter);
    Sequence seq(Sequence::Event,TransitionId::L1Accept,ctime,stamp);
    EvrDatagram datagram(seq, _evtCounter++);

    // for testing
    if ((fe.TimestampHigh & dropPulseMask)==dropPulseMask) {
      printf("Dropping %x\n", fe.TimestampHigh);
      return;
    }

    _outlet.send((char*)&datagram,0,0,_dst);

    if (_evtCounter==_evtStop)
      _done.expired();
  }
  void reset    () { _evtCounter = 0; }
  void enable   (bool e) { _enabled=e; }
  bool enable   () const { return _enabled; }
  void dst      (const Ins& ins) { _dst=ins; }
  void stopAfter(unsigned n) { _evtStop = _evtCounter+n; }
private:
  Evr&         _er;
  DoneTimer&   _done;
  Client       _outlet;
  Ins          _dst;
  unsigned     _evtCounter;
  unsigned     _evtStop;
  bool         _enabled;
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
  EvrEnableAction(Evr& er, DoneTimer& done) :
    EvrAction(er),
    _done    (done) {}
  Transition* fire(Transition* tr) {

    if (l1xmitGlobal->enable()) {
      const EnableEnv& env = static_cast<const EnableEnv&>(tr->env());
      l1xmitGlobal->stopAfter(env.events());
      if (env.timer()) {
	_done.set_duration_ms  (env.duration());
	_done.start();
      }
    }

    unsigned ram=0;
    _er.MapRamEnable(ram,1);
    return tr;
  }
private:
  DoneTimer& _done;
};

class EvrDisableAction : public EvrAction {
public:
  EvrDisableAction(Evr& er, DoneTimer& done) : 
    EvrAction(er), _done(done) {}
  Transition* fire(Transition* tr) {
    unsigned ram=0;
    _er.MapRamEnable(ram,0);

    _done.cancel();
    return tr;
  }
private:
  DoneTimer& _done;
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
  _er(erInfo.board()),_fsm(*new Fsm), _done(new DoneTimer(_fsm)) {

  l1xmitGlobal = new L1Xmitter(_er, *_done);

  _fsm.callback(TransitionId::Map      , new EvrAllocAction(cfg));
  _fsm.callback(TransitionId::Configure,new EvrConfigAction(_er,opcode,cfg));
  _fsm.callback(TransitionId::BeginRun ,new EvrBeginRunAction(_er));
  _fsm.callback(TransitionId::EndRun   ,new EvrEndRunAction  (_er));
  _fsm.callback(TransitionId::Enable   ,new EvrEnableAction (_er, *_done));
  _fsm.callback(TransitionId::Disable  ,new EvrDisableAction(_er, *_done));
  _fsm.callback(TransitionId::L1Accept ,new EvrL1Action(_er));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}

EvrManager::~EvrManager()
{
  delete _done;
}

void EvrManager::drop_pulses(unsigned id)
{
  dropPulseMask = id;
}
