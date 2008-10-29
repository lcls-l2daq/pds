
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
#include "EvgrOpcodes.hh"

using namespace Pds;

class TimeLoader;
static TimeLoader* timeLoaderGlobal;
static EvgrBoardInfo<Evr> *erInfoGlobal;  // yuck

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
        EvgrOpcodes::TsBit1 : EvgrOpcodes::TsBit0;
      unsigned timestamp=numEvtCode;
      _eg.SetSeqRamEvent(ram, numEvtCode, timestamp, opcode);
    } while (++numEvtCode<(NumEvtCodes-1));
    unsigned timestamp=numEvtCode;
    _eg.SetSeqRamEvent(ram, numEvtCode++, timestamp, EvgrOpcodes::TsBitEnd);
  }
  void xmit() {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);
    printf("Received opcode 0x%x timestamp 0x%x/0x%x\n",
           fe.EventCode,fe.TimestampHigh,fe.TimestampLow);
    load();
  }
private:
  enum {SECONDS_1970_TO_1997=852076800};
  enum {NS_PER_FIDUCIAL=2777778};

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
  EvgrEnableAction(TimeLoader& time, Evg& eg, Evr& er) :
    EvgrAction(eg,er),_time(time) {}
  Transition* fire(Transition* tr) {
    _time.load();
    unsigned ram=0;
    _er.MapRamEnable(ram,1);
    return tr;
  }
private:
  TimeLoader& _time;
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

// pairs of timestamps/opcodes, terminated by a 0x7f opcode
static int seqram[][2] = {{4,EvgrOpcodes::L1Accept},{8,EvgrOpcodes::EndOfSequence}};

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
    opcode=EvgrOpcodes::L1Accept;
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

  int pos=0;
  int timestamp;
  do {
    timestamp=seqram[pos][0];
    opcode=seqram[pos][1];
    printf("Programming timestamp %x opcode %x\n",timestamp,opcode);
    _eg.SetSeqRamEvent(ram, pos+TimeLoader::NumEvtCodes,
                       timestamp+TimeLoader::NumEvtCodes, opcode);
    pos++;
  } while (opcode!=0x7f);
  
  // setup properties for multiplexed counter 0
  // to trigger the sequencer at 120Hz.
  static const unsigned EVTCLK_TO_120HZ=991666*120;
  _eg.SetMXCPrescaler(0, EVTCLK_TO_120HZ); // set prescale to 1
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
        timeLoaderGlobal->xmit();
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
  action = new EvgrEnableAction(*timeLoaderGlobal,_eg,_er);
  printf("Enable\n");
  action->fire((Transition*)0);

  _er.IrqAssignHandler(erInfo.filedes(), &evgrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}
