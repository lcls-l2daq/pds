
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
#include "EvgManager.hh"
#include "EvgrOpcode.hh"
#include "EvgrPulseParams.hh"
#include "EvgMasterTiming.hh"
#include "EventSequencerSim.hh"

#include "pds/config/EvrConfigType.hh"

#define BASE_120
#define POSIX_TIME_AT_EPICS_EPOCH 631152000u

using namespace Pds;

static EvgrBoardInfo<Evg> *egInfoGlobal;

static unsigned _opcodecount=0;
static int _ram=0;

static const unsigned EVTCLK_TO_360HZ=991666/3;
static const unsigned TRM_OPCODE=1;
typedef struct {
    unsigned short type;
    unsigned short version;
    unsigned int   mod[6];
    unsigned int   sec;
    unsigned int   nsec;
    unsigned int   avgdone;
    unsigned int   minor;
    unsigned int   major;
    unsigned int   init;
} pattern_t;

class EvgSequenceLoader {
public:
  EvgSequenceLoader(Evg& eg, const EvgMasterTiming& mtime) :
    _eg(eg), _mtime(mtime), _nfid(0x1ec30), _count(0) {
      struct timespec tp;
      memset(&_pattern, 0, sizeof(_pattern));
      _pattern.type = 1;
      _pattern.version = 1;
      clock_gettime(CLOCK_REALTIME, &tp);
      _sec = tp.tv_sec - POSIX_TIME_AT_EPICS_EPOCH;
      _nsec = tp.tv_nsec;
  }
public:
  void set() {
    int ram = 1-_ram;
    int modulo720 = 0;

    _pos = 0;
    unsigned p = 0;

    unsigned step = _mtime.sequences_per_main();

    const unsigned NumEvtCodes=33;
    for(unsigned i=0; i<step; i++) {
        {
          //  prev Timestamp
          unsigned nfidPrev = (_nfid == 0 ? TimeStamp::MaxFiducials-1 : _nfid-1);
          unsigned numEvtCode=0;
          do {
            unsigned mask = (1<<((NumEvtCodes-2)-numEvtCode));
            unsigned opcode = (nfidPrev&mask) ?
                              EvgrOpcode::TsBit1 : EvgrOpcode::TsBit0;
            _set_opcode(ram, opcode, _pos+p);
          } while (++numEvtCode<(NumEvtCodes-1));
          _set_opcode(ram, EvgrOpcode::TsBitEnd, _pos+p);
          p += NumEvtCodes;
        }

        _set_opcode(ram, TRM_OPCODE, _pos+p);
        //p+=99;

        unsigned numEvtCode=0;
        do {
          unsigned mask = (1<<((NumEvtCodes-2)-numEvtCode));
          unsigned opcode = (_nfid&mask) ?
              EvgrOpcode::TsBit1 : EvgrOpcode::TsBit0;
          _set_opcode(ram, opcode, _pos+p);
        } while (++numEvtCode<(NumEvtCodes-1));
        _set_opcode(ram, EvgrOpcode::TsBitEnd, _pos+p);

        const unsigned dbeam = _mtime.fiducials_per_beam();
        //unsigned q = p+36;
        unsigned q = p+67;
        //  Eventcodes
        if (_count%dbeam  ==0) {_set_opcode(ram, 140, q+11850);}

        int  nSeqEvents = 0;
        int* lSeqEvents = NULL;
        _evtSeq.getSeqEvent(_nfid, nSeqEvents, lSeqEvents);

        ////!!debug
        //if (nSeqEvents != 0) printf("** fid 0x%x ", _nfid);

        for (int iEvent=0; iEvent < nSeqEvents; ++iEvent) {
          _set_opcode(ram, lSeqEvents[iEvent], q + _evtSeq.getCodeDelay(lSeqEvents[iEvent]));

          //printf("[%d (%d)] ", lSeqEvents[iEvent], q + _evtSeq.getCodeDelay(lSeqEvents[iEvent]));
        }
        //!!debug
        //if (nSeqEvents != 0) printf("\n");

        if (_nfid%2      ==0) {_set_opcode(ram, 180, q+12944);}
        if (_nfid%3      ==0) {_set_opcode(ram,  40, q+12954);}
        if (_nfid%6      ==0) {_set_opcode(ram,  41, q+12964);}
        if (_nfid%12     ==0) {_set_opcode(ram,  42, q+12974);}
        if (_nfid%36     ==0) {_set_opcode(ram,  43, q+12984);}
        if (_nfid%72     ==0) {_set_opcode(ram,  44, q+12994);}
        if (_nfid%360    ==0) {_set_opcode(ram,  45, q+13004);}
        if (_nfid%(360*2)==0) {_set_opcode(ram,  46, q+13014);}
        if (_nfid%(360*4)==0) {_set_opcode(ram,  47, q+13024);}
        if (_nfid%(360*8)==0) {_set_opcode(ram,  48, q+13034);}
        if (_nfid%(360*2)==360) { modulo720 = 0x8000; }
        p += _mtime.clocks_per_sequence() - NumEvtCodes;

        //  Timestamp
        if (++_nfid >= TimeStamp::MaxFiducials)
          _nfid -= TimeStamp::MaxFiducials;
        ++_count;
      }

      p -= _mtime.clocks_per_sequence() - NumEvtCodes;
      unsigned q = p+72;
      _set_opcode(ram, EvgrOpcode::EndOfSequence, q+14050);

      if ((_count%(360*131072))==step) {
        printf("count %d\n",_count);
        _eg.SeqRamDump(ram);
        printf("===\n");
      }

      /* Now, update and send the data buffer */
      _nsec += 2777778; /* 2.7ms = 360Hz */
      if (_nsec > 1000000000) {
          _nsec -= 1000000000;
          _sec++;
      }

      unsigned _nfid3 = _nfid + 2;
      if (_nfid3 >= TimeStamp::MaxFiducials)
        _nfid3 -= TimeStamp::MaxFiducials;
      _pattern.sec = _sec;
      _pattern.nsec = (_nsec & 0xfffe0000) | _nfid3;
      if (_pattern.nsec > 1000000000) {
        /* Sigh.  Adding the fiducial in the low bits moved us over 1e9 ns. */
        _pattern.sec = _sec + 1;
        _pattern.nsec -= (1000000000 & 0xfffe0000);
      }
      _pattern.mod[0] = modulo720;

      /* Sigh.  We swizzle so we can unswizzle later. */
      unsigned int buf[sizeof(_pattern)/sizeof(int)], *buf2 = (unsigned int *)&_pattern;
      for (unsigned int i = 0; i < sizeof(_pattern)/sizeof(int); i++)
        buf[i] = be32_to_cpu(buf2[i]);
      _eg.SendDBuf((char *)&buf, sizeof(buf));

      ////!!debug
      //if (nSeqEvents != 0) printf("** fid 0x%x ", _nfid);

      // re-arm the event sequence (then it will be triggered by internal clock or external trigger)
      int enable=1, single=1, recycle=0, reset=0;
      int trigsel=(_mtime.internal_main()? C_EVG_SEQTRIG_MXC_BASE : C_EVG_SEQTRIG_ACINPUT);

      _eg.SeqRamCtrl(ram, enable, single, recycle, reset, trigsel);
      _ram = ram;
    }
private:
  void _set_opcode(int ram, unsigned opcode, unsigned pos) {
    _eg.SetSeqRamEvent(ram, _pos, pos, opcode);
    _pos++;
  }
  Evg&                    _eg;
  const EvgMasterTiming&  _mtime;
  EventSequencerSim       _evtSeq;
  unsigned                _nfid;
  unsigned                _count;
  unsigned                _pos;
  pattern_t _pattern;
  int       _sec;
  int       _nsec;
};

static EvgSequenceLoader*   sequenceLoaderGlobal;

class EvgAction : public Action {
protected:
  EvgAction(Evg& eg) : _eg(eg) {}
  Evg& _eg;
};

class EvgEnableAction : public EvgAction {
public:
  EvgEnableAction(Evg& eg) :
    EvgAction(eg) {}
  Transition* fire(Transition* tr) {
    sequenceLoaderGlobal->set();
    return tr;
  }
};

class EvgDisableAction : public EvgAction {
public:
  EvgDisableAction(Evg& eg) : EvgAction(eg) {}
  Transition* fire(Transition* tr) {
    return tr;
  }
};

class EvgBeginRunAction : public EvgAction {
public:
  EvgBeginRunAction(Evg& eg) : EvgAction(eg) {}
  Transition* fire(Transition* tr) {
    _eg.IrqEnable((1 << C_EVG_IRQ_MASTER_ENABLE) | (3 << C_EVG_IRQFLAG_SEQSTOP));
    _eg.Enable(1);
    return tr;
  }
};

class EvgEndRunAction : public EvgAction {
public:
  EvgEndRunAction(Evg& eg) : EvgAction(eg) {}
  Transition* fire(Transition* tr) {
    _eg.Enable(0);
    _eg.IrqEnable(0);
    return tr;
  }
};

class EvgConfigAction : public EvgAction {
public:
  EvgConfigAction(Evg& eg, const EvgMasterTiming& mtim) :
    EvgAction(eg),
    mtime(mtim) {}

  Transition* fire(Transition* tr) {
    int trigsel=(mtime.internal_main()? C_EVG_SEQTRIG_MXC_BASE : C_EVG_SEQTRIG_ACINPUT);

    printf("Configuring evg\n");
    _eg.Reset();
    _eg.SeqRamCtrl(0, 0, 0, 0, 0, trigsel); // Disable both sequence rams!
    _eg.SeqRamCtrl(1, 0, 0, 0, 0, trigsel);

    for(int ram=0; ram<2; ram++) {
        _eg.SetSeqRamEvent(ram, 0, 0, EvgrOpcode::EndOfSequence);
    }

    _eg.SetRFInput(0,C_EVG_RFDIV_4);

    if (mtime.internal_main()) {
      // setup properties for multiplexed counter 0
      // to trigger the sequencer at 360Hz (so we can "stress" the system).

      _eg.SetMXCPrescaler(0, mtime.clocks_per_sequence()*mtime.sequences_per_main());
      _eg.SyncMxc();
    }
    else {
      int bypass=1, sync=0, div=0, delay=0;
      _eg.SetACInput(bypass, sync, div, delay);

      int map = -1;
      _eg.SetACMap(map);
    }

    _eg.SetDBufMode(1);

    return tr;
  }
public:
  const EvgMasterTiming& mtime;
};

extern "C" {
  void evgmgr_sig_handler(int parm)
  {
    static Evg& eg  = egInfoGlobal->board();
    static int fdEg = egInfoGlobal->filedes();
    
    int flags = eg.GetIrqFlags();
    if (flags & (3 << C_EVG_IRQFLAG_SEQSTOP)) {
        sequenceLoaderGlobal->set();
        _opcodecount++;

        eg.ClearIrqFlags(3 << C_EVG_IRQFLAG_SEQSTOP);
        eg.IrqHandled(fdEg);
    }
  }
}

Appliance& EvgManager::appliance() {return _fsm;}

EvgManager::EvgManager(
    EvgrBoardInfo<Evg> &egInfo,
    const EvgMasterTiming& mtime) :
  _eg(egInfo.board()),
  _fsm(*new Fsm)
{

  sequenceLoaderGlobal   = new EvgSequenceLoader(_eg,mtime);

  _eg.IrqAssignHandler(egInfo.filedes(), &evgmgr_sig_handler);
  egInfoGlobal = &egInfo;

  Action* action;
  action = new EvgConfigAction(_eg,mtime);
  action->fire((Transition*)0);
  action = new EvgBeginRunAction(_eg);
  action->fire((Transition*)0);
  action = new EvgEnableAction(_eg);
  action->fire((Transition*)0);
}

unsigned EvgManager::opcodecount() const {return _opcodecount;}
