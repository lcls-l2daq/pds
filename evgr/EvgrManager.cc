
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
#include "EvgrPulseParams.hh"
#include "EvgMasterTiming.hh"

#include "pds/config/EvrConfigType.hh"

#define BASE_120

using namespace Pds;

static EvgrBoardInfo<Evr> *erInfoGlobal;

static unsigned _opcodecount=0;
static unsigned _lastopcode=0;
static int _ram=0;

static const unsigned EVTCLK_TO_360HZ=991666/3;
static const unsigned TRM_OPCODE=1;
static const unsigned IRQ_OPCODE=2;

class SequenceLoader {
public:
  SequenceLoader(Evr& er, Evg& eg, const EvgMasterTiming& mtime) :
    _er(er), _eg(eg), _mtime(mtime), _nfid(0), _count(0) {}
public:
  void set() {
    //    int ram = 1-_ram;
    int ram = _ram;

    _pos = 0;
    unsigned p = 0;

    unsigned step = _mtime.sequences_per_main();

    for(unsigned i=0; i<step; i++) {

        _set_opcode(ram, TRM_OPCODE, _pos+p);

        //  Timestamp
        _nfid++;
        if (_nfid > TimeStamp::MaxFiducials)
          _nfid -= TimeStamp::MaxFiducials;

        const unsigned NumEvtCodes=33;
        unsigned numEvtCode=0;
        do {
          unsigned mask = (1<<((NumEvtCodes-2)-numEvtCode));
          unsigned opcode = (_nfid&mask) ?
              EvgrOpcode::TsBit1 : EvgrOpcode::TsBit0;
          _set_opcode(ram, opcode, _pos+p);
        } while (++numEvtCode<(NumEvtCodes-1));
        _set_opcode(ram, EvgrOpcode::TsBitEnd, _pos+p);

        unsigned q = p+36;
        //  Eventcodes
        if (_count%2      ==0) {_set_opcode(ram, 180, q+12944);}
        if (_count%3      ==0) {_set_opcode(ram,  40, q+12954);}
        if (_count%6      ==0) {_set_opcode(ram,  41, q+12964);}
        if (_count%12     ==0) {_set_opcode(ram,  42, q+12974);}
        if (_count%36     ==0) {_set_opcode(ram,  43, q+12984);}
        if (_count%72     ==0) {_set_opcode(ram,  44, q+12994);}
        if (_count%360    ==0) {_set_opcode(ram,  45, q+13004);}
        if (_count%(360*2)==0) {_set_opcode(ram,  46, q+13014);}
        if (_count%(360*4)==0) {_set_opcode(ram,  47, q+13024);}
        if (_count%(360*8)==0) {_set_opcode(ram,  48, q+13034);}
        p += _mtime.clocks_per_sequence();
        _count++;
      }

      p -= _mtime.clocks_per_sequence();
      _set_opcode(ram, IRQ_OPCODE, p+13043);
      _set_opcode(ram, EvgrOpcode::EndOfSequence, p+13044);
      _set_opcode(ram, 0, 0);

      if ((_count%(360*131072))==step) {
        printf("count %d\n",_count);
        _eg.SeqRamDump(ram);
        printf("===\n");
      }

      //     int enable=1; int single=1; int recycle=0; int reset=0;
      //     int trigsel=C_EVG_SEQTRIG_MXC_BASE;
      //     _eg.SeqRamCtrl(ram, enable, single, recycle, reset, trigsel);
      //     _ram = ram;
    }
private:
  void _set_opcode(int ram, unsigned opcode, unsigned pos) {
    _eg.SetSeqRamEvent(ram, _pos, pos, opcode);
    _pos++;
  }
  Evr& _er;
  Evg& _eg;
  const EvgMasterTiming& _mtime;
  unsigned _nfid;
  unsigned _count;
  unsigned _pos;
};

static SequenceLoader*   sequenceLoaderGlobal;

class EvgrAction : public Action {
protected:
  EvgrAction(Evg& eg, Evr& er) : _eg(eg),_er(er) {}
  Evg& _eg;
  Evr& _er;
};

class EvgrEnableAction : public EvgrAction {
public:
  EvgrEnableAction(Evg& eg, Evr& er) :
    EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    _ram=0;
    sequenceLoaderGlobal->set();

    int ram=0;    _er.MapRamEnable(ram,1);

    return tr;
  }
};

class EvgrDisableAction : public EvgrAction {
public:
  EvgrDisableAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    unsigned ram;
    ram=0;    _er.MapRamEnable(ram,0);
    ram=1;    _er.MapRamEnable(ram,0);
    return tr;
  }
};

class EvgrBeginRunAction : public EvgrAction {
public:
  EvgrBeginRunAction(Evg& eg, Evr& er) : EvgrAction(eg,er) {}
  Transition* fire(Transition* tr) {
    unsigned ram;
    ram=0;    _er.MapRamEnable(ram,0);
    ram=1;    _er.MapRamEnable(ram,0);
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
  EvgrConfigAction(Evg& eg, Evr& er, 
                   unsigned n, EvgrPulseParams* p,
                   const EvgMasterTiming& mtim) : 
    EvgrAction(eg,er),
    mtime(mtim) 
  {
    npulses = n;
    pulse = p;
  }

  Transition* fire(Transition* tr) {
    printf("Configuring evg\n");
    _eg.Reset();
    for(int ram=0; ram<2; ram++) {
      for(unsigned pos=0; pos<EVTCLK_TO_360HZ; pos++)
        _eg.SetSeqRamEvent(ram, pos, 0, 0);
    }

    _er.Reset();
    _eg.SetRFInput(0,C_EVG_RFDIV_4);

    // setup map ram
    printf("Configuring evr\n");
    int ram=0; int enable=1;
    _er.MapRamEnable(ram,0);
    _er.SetFIFOEvent(ram, IRQ_OPCODE, enable);

    for(unsigned i=0; i<npulses; i++) {
      printf("Configuring pulse %d  eventcode %d  delay %d  width %d  output %d\n",
             i, pulse[i].eventcode, pulse[i].delay, pulse[i].width, pulse[i].output);
      _er.SetPulseMap(ram, pulse[i].eventcode, i, -1, -1);
      _er.SetPulseProperties(i, pulse[i].polarity, 0, 0, 1, 1);
      _er.SetPulseParams(i,1,pulse[i].delay,pulse[i].width);
      _er.SetUnivOutMap( pulse[i].output, i);
    }

    _er.DumpPulses(npulses);

    if (mtime.internal_main()) {
      // setup properties for multiplexed counter 0
      // to trigger the sequencer at 360Hz (so we can "stress" the system).

      _eg.SetMXCPrescaler(0, mtime.clocks_per_sequence()*mtime.sequences_per_main());
      _eg.SyncMxc();

      int single=0; int recycle=0; int reset=0;
      int trigsel=C_EVG_SEQTRIG_MXC_BASE;
      _eg.SeqRamCtrl(ram, enable, single, recycle, reset, trigsel);
    }
    else {
      int bypass=0, sync=0, div=0, delay=0;
      _eg.SetACInput(bypass, sync, div, delay);
      
      int map = -1;
      _eg.SetACMap(map);

      int single=0; int recycle=0; int reset=0;
      int trigsel=C_EVG_SEQTRIG_ACINPUT;
      _eg.SeqRamCtrl(ram, enable, single, recycle, reset, trigsel);
    }

    return tr;
  }
public:
  unsigned npulses;
  EvgrPulseParams* pulse;
  const EvgMasterTiming& mtime;
};

extern "C" {
  void evgrmgr_sig_handler(int parm)
  {
    Evr& er = erInfoGlobal->board();
    int flags = er.GetIrqFlags();
    if (flags & EVR_IRQFLAG_EVENT)
      {
	FIFOEvent fe;
	er.GetFIFOEvent(&fe);

	if ((_opcodecount%(360*8192))==0)
	  printf("ncodes %d  last %d\n",_opcodecount,_lastopcode);

	_opcodecount++;
	_lastopcode = fe.EventCode;

	sequenceLoaderGlobal->set();

        er.ClearIrqFlags(EVR_IRQFLAG_EVENT);
      }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance& EvgrManager::appliance() {return _fsm;}

EvgrManager::EvgrManager(
    EvgrBoardInfo<Evg> &egInfo,
    EvgrBoardInfo<Evr> &erInfo,
    const EvgMasterTiming& mtime,
    unsigned npulses,
    EvgrPulseParams* pulse) :
  _eg(egInfo.board()),
  _er(erInfo.board()),
  _fsm(*new Fsm),
  _nplss(npulses),
  _pls(pulse)
{

  sequenceLoaderGlobal   = new SequenceLoader(_er,_eg,mtime);
    
//   _fsm.callback(TransitionId::Configure,new EvgrConfigAction(_eg,_er));
//   _fsm.callback(TransitionId::BeginRun,new EvgrBeginRunAction(_eg,_er));
//   _fsm.callback(TransitionId::Enable,new EvgrEnableAction(*timeLoaderGlobal,_eg,_er));
//   _fsm.callback(TransitionId::EndRun,new EvgrEndRunAction(_eg,_er));
//   _fsm.callback(TransitionId::Disable,new EvgrDisableAction(_eg,_er));

  Action* action;
  action = new EvgrConfigAction(_eg,_er,_nplss,pulse,mtime);
  action->fire((Transition*)0);
  action = new EvgrBeginRunAction(_eg,_er);
  action->fire((Transition*)0);
  action = new EvgrEnableAction(_eg,_er);
  action->fire((Transition*)0);

  _er.IrqAssignHandler(erInfo.filedes(), &evgrmgr_sig_handler);
  erInfoGlobal = &erInfo;
}

unsigned EvgrManager::opcodecount() const {return _opcodecount;}
unsigned EvgrManager::lastopcode() const {return _lastopcode;}
