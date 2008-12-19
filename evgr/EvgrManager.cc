
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

class TimeLoader {
public:
  TimeLoader(Evg& eg, Evr& er) : _eg(eg),_er(er) {}
  virtual ~TimeLoader() {}
  enum {NumEvtCodes=33};
  void load() {

    // we kludge the number of fiducials for testing.  In principle, stephanie
    // should be able to calculate.
    timespec ts;
    clock_gettime (CLOCK_REALTIME, &ts);
    ts.tv_sec-=SECONDS_1970_TO_1997;
    // convert to fiducials, approximately;
    unsigned long long nsec = ((unsigned long long)ts.tv_sec*1000000000 + ts.tv_nsec);
    unsigned long long fid = nsec/(NS_PER_FIDUCIAL);

    int numEvtCode=0;
    int ram=0;
    //we send down MSB first
    do {
      unsigned opcode = (fid&(1<<((NumEvtCodes-1)-numEvtCode))) ?
        EvgrOpcode::TsBit1 : EvgrOpcode::TsBit0;
      unsigned timestamp=numEvtCode;
      _eg.SetSeqRamEvent(ram, numEvtCode, timestamp, opcode);
    } while (++numEvtCode<(NumEvtCodes-1));
    unsigned timestamp=numEvtCode;
    _eg.SetSeqRamEvent(ram, numEvtCode++, timestamp, EvgrOpcode::TsBitEnd);
  }
  void set() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    load();
  }
private:
  enum {SECONDS_1970_TO_1997=852076800};
  enum {NS_PER_FIDUCIAL=2777778};

  Evg& _eg;
  Evr& _er;
};

class OpcodeLoader {
public:
  OpcodeLoader(Evg& eg, Evr& er) : _count(0),_eg(eg),_er(er) {}
  void set() {
    int ram=0;
    int pos=TimeLoader::NumEvtCodes;
    _count++;

    _eg.SetSeqRamEvent(ram, pos, pos, 240); pos++;
    if (_count%2==0) {_eg.SetSeqRamEvent(ram, pos, pos, EvgrOpcode::L1Accept); pos++;}
    if (_count%4==0) {_eg.SetSeqRamEvent(ram, pos, pos, 60); pos++;}
    if (_count%8==0) {_eg.SetSeqRamEvent(ram, pos, pos, 30); pos++;}
    if (_count%16==0) {_eg.SetSeqRamEvent(ram, pos, pos, 15); pos++;}
    if (_count%24==0) {_eg.SetSeqRamEvent(ram, pos, pos, 10); pos++;}
    if (_count%48==0) {_eg.SetSeqRamEvent(ram, pos, pos, 5); pos++;}
    if (_count%240==0) {_eg.SetSeqRamEvent(ram, pos, pos, 1); pos++;}
    _eg.SetSeqRamEvent(ram, pos, pos, EvgrOpcode::EndOfSequence);
//     _eg.SeqRamDump(0);
  }
private:
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
    int ram=0; int opcode=9; int enable=1;
    _er.MapRamEnable(ram,0);
    _er.SetFIFOEvent(ram, opcode, enable);
//     opcode=EvgrOpcode::L1Accept;
    opcode=240;
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
    // to trigger the sequencer at 240Hz (so we can "stress" the system).
    static const unsigned EVTCLK_TO_240HZ=991666/2;
    _eg.SetMXCPrescaler(0, EVTCLK_TO_240HZ); // set prescale to 1
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
  printf("Config\n");
  action->fire((Transition*)0);
  action = new EvgrBeginRunAction(_eg,_er);
  printf("Begin\n");
  action->fire((Transition*)0);
  action = new EvgrEnableAction(*timeLoaderGlobal,*opcodeLoaderGlobal,_eg,_er);
  printf("Enable\n");
  action->fire((Transition*)0);

  _er.IrqAssignHandler(erInfo.filedes(), &evgrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}
