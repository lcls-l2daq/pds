
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

#define POSIX_TIME_AT_EPICS_EPOCH 631152000u

using namespace Pds;

static int egfd = -1;
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
      memset(&_event, 0, sizeof(_event));
      memset(&_pattern, 0, sizeof(_pattern));
      _pattern.type = 1;
      _pattern.version = 1;
      clock_gettime(CLOCK_REALTIME, &tp);
      _sec = tp.tv_sec - POSIX_TIME_AT_EPICS_EPOCH;
      _nsec = tp.tv_nsec;
  }
public:
  void set() {
    int ram = 0;
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
        unsigned q = p+69;
        //  Eventcodes
        if (_count%dbeam       ==0) {_set_opcode(ram, 140, q+11850);}
        if (_count%(2*dbeam)   ==0) {_set_opcode(ram, 141, q+11851);}
        if (_count%(4*dbeam)   ==0) {_set_opcode(ram, 142, q+11852);}
        if (_count%(12*dbeam)  ==0) {_set_opcode(ram, 143, q+11853);}
        if (_count%(24*dbeam)  ==0) {_set_opcode(ram, 144, q+11854);}
        if (_count%(120*dbeam) ==0) {_set_opcode(ram, 145, q+11855);}
        if (_count%(240*dbeam) ==0) {_set_opcode(ram, 146, q+11856);}

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

        _set_opcode(ram, 9, q+12900);
        if (_nfid%2      ==0) {_set_opcode(ram,  10, q+12951);} // not in real MCC event code
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
        SeqRamDump(ram);
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
      unsigned int *buf = (unsigned int *)&_pattern;
      for (unsigned int i = 0; i < sizeof(_pattern)/sizeof(int); i++)
        _event.pattern[i] = be32_to_cpu(buf[i]);

      ////!!debug
      //if (nSeqEvents != 0) printf("** fid 0x%x ", _nfid);

      ioctl(egfd, EV_IOCQEVT, &_event);
    }
private:
  void _set_opcode(int ram, unsigned opcode, unsigned pos) {
    _event.seqram[_pos].timestamp = be32_to_cpu(pos);
    _event.seqram[_pos].eventcode = be32_to_cpu(opcode);
    _pos++;
  }

  void SeqRamDump(int ram) {
    int pos;
    for (pos = 0; pos < EVG_SEQRAM_SIZE; pos++) {
      if (_event.seqram[pos].eventcode)
        printf("Ram%d: Timestamp %08x Code %02x\n",
       ram,
       be32_to_cpu(_event.seqram[pos].timestamp),
       be32_to_cpu(_event.seqram[pos].eventcode));
      if (be32_to_cpu(_event.seqram[pos].eventcode) == EvgrOpcode::EndOfSequence)
        break;
    }
  }
  Evg&                    _eg;
  const EvgMasterTiming&  _mtime;
  EventSequencerSim       _evtSeq;
  unsigned                _nfid;
  unsigned                _count;
  unsigned                _pos;
  pattern_t _pattern;
  evg_event_t _event;
  int       _sec;
  int       _nsec;
};

static EvgSequenceLoader*   sequenceLoaderGlobal;

Appliance& EvgManager::appliance() {return _fsm;}

EvgManager::EvgManager(
    EvgrBoardInfo<Evg> &egInfo,
    const EvgMasterTiming& mtime) :
  _eg(egInfo.board()),
  _fsm(*new Fsm)
{

  egfd = egInfo.filedes();
  ioctl(egfd, EV_IOCKINIT, 0);
  if (!mtime.internal_main())
      ioctl(egfd, EV_IOCSETEXTTRIG, 1);
  sequenceLoaderGlobal   = new EvgSequenceLoader(_eg,mtime);

  for (;;)
    sequenceLoaderGlobal->set();
}

