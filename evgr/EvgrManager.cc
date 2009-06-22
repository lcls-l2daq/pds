
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <new>
#include <pthread.h>
#include <semaphore.h>

#include "evgr/evg/evg.hh"
#include "evgr/evr/evr.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "EvgrBoardInfo.hh"
#include "EvgrManager.hh"
#include "EvgrOpcode.hh"

using namespace Pds;

class TimeLoader;
class OpcodeLoader;
static TimeLoader* timeLoaderGlobal;
static OpcodeLoader* opcodeLoaderGlobal;
static EvgrBoardInfo<Evr> *erInfoGlobal;

static unsigned _opcodecount=0;
static unsigned _lastopcode=0;

class TimeLoader {
public:
  TimeLoader(Evg& eg, Evr& er) : _eg(eg),_er(er),_nfid(0) {}
  virtual ~TimeLoader() {}
  enum {NumEvtCodes=33};
  void load() {
    _nfid++; _nfid&=((1<<Pds::TimeStamp::NumFiducialBits)-1);
    int numEvtCode=0;
    int ram=0;
    //we send down MSB first
    do {
      unsigned mask = (1<<((NumEvtCodes-2)-numEvtCode)); 
      unsigned opcode = (_nfid&mask) ?
        EvgrOpcode::TsBit1 : EvgrOpcode::TsBit0;
      unsigned timestamp=numEvtCode;
      _eg.SetSeqRamEvent(ram, numEvtCode, timestamp, opcode);
    } while (++numEvtCode<(NumEvtCodes-1));
    unsigned timestamp=numEvtCode;
    _eg.SetSeqRamEvent(ram, numEvtCode, timestamp, EvgrOpcode::TsBitEnd);
  }
  void set() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    _opcodecount++;
    _lastopcode = fe.EventCode;
    load();
  }
private:
  Evg& _eg;
  Evr& _er;
  unsigned _nfid;
};

class OpcodeLoader {
public:
  OpcodeLoader(Evg& eg, Evr& er) : _count(0),_eg(eg),_er(er) {}
  void set() {
    int ram=0;
    int pos=TimeLoader::NumEvtCodes;
    _count++; _count%=360;

    _eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 181); pos++;
    if (_count%2==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 180); pos++;}
    if (_count%3==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, EvgrOpcode::L1Accept); pos++;}
    if (_count%6==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 60); pos++;}
    if (_count%9==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 40); pos++;}
    if (_count%12==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 30); pos++;}
    if (_count%24==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 15); pos++;}
    if (_count%36==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 10); pos++;}
    if (_count%72==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 5); pos++;}
    if (_count%360==0) {_eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, 1); pos++;}
    _eg.SetSeqRamEvent(ram, pos, pos+WaitForTimestamp, EvgrOpcode::EndOfSequence);
  }
private:
  enum {WaitForTimestamp=8}; // timestamp takes some clock ticks to take effect
  unsigned _count;
  Evg& _eg;
  Evr& _er;
};

class EvgrAction : public Action {
protected:
  EvgrAction(Evg& eg, Evr& er) : _eg(eg),_er(er) {}
  Evg& _eg;
  Evr& _er;
};

class EvgrEnableAction : public EvgrAction {
public:
  EvgrEnableAction(TimeLoader& time, OpcodeLoader& opcodes, Evg& eg, Evr& er) :
    EvgrAction(eg,er),_time(time),_opcodes(opcodes) {}
  Transition* fire(Transition* tr) {
    _time.load();
    _opcodes.set();
    unsigned ram=0;
    _er.MapRamEnable(ram,1);
    return tr;
  }
private:
  TimeLoader& _time;
  OpcodeLoader& _opcodes;
};

class EvgrDisableAction : public EvgrAction {
public:
  EvgrDisableAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    unsigned ram=0;
    _er.MapRamEnable(ram,0);
    return tr;
  }
};

class EvgrBeginRunAction : public EvgrAction {
public:
  EvgrBeginRunAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    unsigned ram=0;
    _er.MapRamEnable(ram,0);
    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    _er.EnableFIFO(1);
    _er.Enable(1);
    _eg.Enable(1);
    return tr;
  }
};

class EvgrEndRunAction : public EvgrAction {
public:
  EvgrEndRunAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    _eg.Enable(0);
    _er.IrqEnable(0);
    _er.Enable(0);
    _er.EnableFIFO(0);
    return tr;
  }
};

class EvgrConfigAction : public EvgrAction {
public:
  EvgrConfigAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    printf("Configuring evg/evr\n");
    _eg.Reset();
    _er.Reset();
    _eg.SetRFInput(0,C_EVG_RFDIV_4);

    // setup map ram
    int ram=0; int enable=1;
    _er.MapRamEnable(ram,0);
    int opcode=181; // for testing, use the highest rate opcode (360Hz).
    _er.SetFIFOEvent(ram, opcode, enable);
    int trig=0; int set=-1; int clear=-1;
    _er.SetPulseMap(ram, opcode, trig, set, clear);

    int pulse = 0; int presc = 1; int delay = 0; int width = 16;
    int polarity=0;  int map_reset_ena=0; int map_set_ena=0; int map_trigger_ena=1;
    _er.SetPulseProperties(pulse, polarity, map_reset_ena, map_set_ena, map_trigger_ena,
                           enable);
    _er.SetPulseParams(pulse,presc,delay,width);

    // map pulse generator 0 to front panel output 0
    _er.SetFPOutMap(0,0);

    // setup properties for multiplexed counter 0
    // to trigger the sequencer at 360Hz (so we can "stress" the system).
    static const unsigned EVTCLK_TO_360HZ=991666/3;
    _eg.SetMXCPrescaler(0, EVTCLK_TO_360HZ); // set prescale to 1
    _eg.SyncMxc();

    enable=1; int single=0; int recycle=0; int reset=0;
    int trigsel=C_EVG_SEQTRIG_MXC_BASE;
    _eg.SeqRamCtrl(ram, enable, single, recycle, reset, trigsel);

    return tr;
  }
};

extern "C" {
  void evgrmgr_sig_handler(int parm)
  {
    Evr& er = erInfoGlobal->board();
    int flags = er.GetIrqFlags();
    if (flags & EVR_IRQFLAG_EVENT)
      {
        timeLoaderGlobal->set();
        opcodeLoaderGlobal->set();
        er.ClearIrqFlags(EVR_IRQFLAG_EVENT);
      }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance& EvgrManager::appliance() {return _fsm;}

EvgrManager::EvgrManager(EvgrBoardInfo<Evg> &egInfo, EvgrBoardInfo<Evr> &erInfo) :
  _eg(egInfo.board()),_er(erInfo.board()),_fsm(*new Fsm) {

  timeLoaderGlobal = new TimeLoader(_eg,_er);
  opcodeLoaderGlobal = new OpcodeLoader(_eg,_er);
//   _fsm.callback(TransitionId::Configure,new EvgrConfigAction(_eg,_er));
//   _fsm.callback(TransitionId::BeginRun,new EvgrBeginRunAction(_eg,_er));
//   _fsm.callback(TransitionId::Enable,new EvgrEnableAction(*timeLoaderGlobal,_eg,_er));
//   _fsm.callback(TransitionId::EndRun,new EvgrEndRunAction(_eg,_er));
//   _fsm.callback(TransitionId::Disable,new EvgrDisableAction(_eg,_er));

  Action* action;
  action = new EvgrConfigAction(_eg,_er);
  action->fire((Transition*)0);
  action = new EvgrBeginRunAction(_eg,_er);
  action->fire((Transition*)0);
  action = new EvgrEnableAction(*timeLoaderGlobal,*opcodeLoaderGlobal,_eg,_er);
  action->fire((Transition*)0);

  _er.IrqAssignHandler(erInfo.filedes(), &evgrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}

unsigned EvgrManager::opcodecount() const {return _opcodecount;}
unsigned EvgrManager::lastopcode() const {return _lastopcode;}
