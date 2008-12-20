
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
#include "pds/service/Client.hh"
#include "EvgrBoardInfo.hh"
#include "EvrManager.hh"
#include "EvgrOpcode.hh"

using namespace Pds;

class L1Xmitter;
static L1Xmitter* l1xmitGlobal;
static EvgrBoardInfo<Evr> *erInfoGlobal;  // yuck

class L1Xmitter {
public:
  L1Xmitter(Evr& er, unsigned partition) :
    _er(er),_partition(partition),_outlet(sizeof(EvrDatagram),0),
    _evtCounter(0) {}
  void xmit() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ClockTime ctime(ts.tv_sec,ts.tv_nsec);
    Sequence seq(Sequence::Event,TransitionId::L1Accept,ctime,
                 fe.TimestampLow,fe.TimestampHigh);
    EvrDatagram datagram(seq, _evtCounter++);
    Ins dst(StreamPorts::event(_partition,Level::Segment));
    _outlet.send((char*)&datagram,0,0,dst);
//     printf("Received opcode 0x%x timestamp 0x%x/0x%x count %d\n",
//            fe.EventCode,fe.TimestampHigh,fe.TimestampLow,_evtCounter-1);
  }
  void reset() { _evtCounter = 0; }
private:
  Evr& _er;
  unsigned _partition;
  Client   _outlet;
  unsigned _evtCounter;
};

class EvrAction : public Action {
protected:
  EvrAction(Evr& er) : _er(er) {}
  Evr& _er;
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
  EvrConfigAction(Evr& er,EvgrOpcode::Opcode opcode) :
    EvrAction(er),_opcode(opcode) {}
  Transition* fire(Transition* tr) {
    printf("Configuring evr\n");
    _er.Reset();

    // setup map ram
    int ram=0; int enable=1;
    _er.MapRamEnable(ram,0);
    _er.SetFIFOEvent(ram, _opcode, enable);

    // acqiris pulse configuration
    { int pulse = 0; int presc = 1; int delay = 0; int width = 16;
      int polarity=0;  int map_reset_ena=0; int map_set_ena=0; int map_trigger_ena=1;
      int trig=0; int set=-1; int clear=-1;
      _er.SetPulseMap(ram, _opcode, trig, set, clear);
      _er.SetPulseProperties(pulse, polarity, map_reset_ena, map_set_ena, map_trigger_ena,
			     enable);
      _er.SetPulseParams(pulse,presc,delay,width);
      
      // map pulse generator 0 to front panel output 0
      _er.SetFPOutMap(0,0);
    }
    // opal1000 pulse configuration
    { int pulse = 1; int presc = 1; int delay = 0; int width = (1<<16)-1;
      int polarity=0;  int map_reset_ena=0; int map_set_ena=0; int map_trigger_ena=1;
      int trig=1; int set=-1; int clear=-1;
      _er.SetPulseMap(ram, _opcode, trig, set, clear);
      _er.SetPulseProperties(pulse, polarity, map_reset_ena, map_set_ena, map_trigger_ena,
			     enable);
      _er.SetPulseParams(pulse,presc,delay,width);
      
      // map pulse generator 1 to front panel output 2
      _er.SetFPOutMap(2,1);
    }
    l1xmitGlobal->reset();
    return tr;
  }
private:
  EvgrOpcode::Opcode _opcode;
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

EvrManager::EvrManager(EvgrBoardInfo<Evr> &erInfo, unsigned partition,
                       EvgrOpcode::Opcode opcode) :
  _er(erInfo.board()),_fsm(*new Fsm) {

  l1xmitGlobal = new L1Xmitter(_er,partition);
  _fsm.callback(TransitionId::Configure,new EvrConfigAction(_er,opcode));
  _fsm.callback(TransitionId::BeginRun,new EvrBeginRunAction(_er));
  _fsm.callback(TransitionId::Enable,new EvrEnableAction(_er));
  _fsm.callback(TransitionId::EndRun,new EvrEndRunAction(_er));
  _fsm.callback(TransitionId::Disable,new EvrDisableAction(_er));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}
