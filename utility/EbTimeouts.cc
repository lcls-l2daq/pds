#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

// Algorithm description:
// 
// Names: 
// 
// - tl = minimum time required by level l to process transition
// - dl = maximum variation required by level l to process transition
// - Min() = minimum over the various crates in the partition
// - Max() = maximum over the various crates in the partition
// - Timeout = Slowest time to get to the EB _minus_
// 	    Fastest time to get to the EB
// 
// Level	Fastest			Slowest
// 		
// Frag		ts			ts+ds
// Event	Min(ts+tf)		Max(ts+ds+tf+df)
// Control	Max(ts+tf)+te		Max(ts+ds+tf+df)+te+de+Te
// Sequencer	Max(ts+tf)+te+tc	Max(ts+ds+tf+df)+te+de+tc+Te+Tc
// 
// 
// Semplications in the algorithm:
// 
// 1. only one parameter to describe a level: tl+dl -> dl, tl -> 0 (note
// that the fastest time is always zero with this semplification)
// 
// 2. Max(ts+ds+tf+df) = Max(ts+ds)+Max(tf+df)
// 
// Level	Timeout
// 		
// Frag		ds
// Event	Max(ds+df)
// Control	Max(ds+df)+de +Max(ds+df)
// Sequencer	Max(ds+df)+de +Max(ds+df) +Max(ds+df)+de+Max(ds+df)

// For some occurrences the timeout mechanism is the only way to be
// flushed by the event builder, hence needs to be short if we want
// the occurrence go through the system rapidily
static const int framework_tmo = 1000; // 1 s
static const int occurence_tmo = 200; // 200 ms

// This is the default number of times the event builder will expire
// before timing out
static const int default_timeout = 1;
static const int default_event   = 8;

// Special timeouts for some transitions in the FrameWork stream
struct SpecialDelta {
  EventId::Value event;
  unsigned levels;
  unsigned crates;
  int timeouts;
};

// Note:
//   0x000003ff EMC
//   0x00000400 EMT
//   0x00000800 DCT
//   0x00001000 GLT
//   0x0001e000 DCH
//   0x00020000 IFR
//   0x001c0000 DRC
//   0x0fe00000 SVT
//   0x10000000 FCT
//   0x20000000 ELP

static const unsigned AllLevels = 0xffffffff;
static const unsigned AllCrates = 0xffffffff;

static SpecialDelta special_delta[] = {
  {EventId::L1Accept,  AllLevels,            AllCrates,   2}
};

static int delta(unsigned crates, 
		 StreamParams::StreamType stream, 
		 Level::Type level,
		 Sequence::Type t, 
		 Sequence::Service s) 
{
  if (stream != StreamParams::FrameWork || t != Sequence::Event) {
    return default_timeout;
  }

  // Look for special timeouts for some transitions in the FrameWork stream
  EventId::Value event = 
    EventId::Value(s);
  int delta = 0;
  SpecialDelta* special = special_delta;
  do {
    if (special->event == event) {
      if (special->levels & (1<<level)) {
	if (special->crates & crates) {
	  if (special->timeouts > delta) delta = special->timeouts;
	}
      }
    }
    ++special;
  } while (special->timeouts);

  return delta ? delta : default_event;
}

EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts) 
  : _duration(ebtimeouts._duration)
{
  const int array_size = Sequence::NumberOfServices*
    Sequence::NumberOfTypes*sizeof(_tmos[0]);
  memcpy(_tmos, ebtimeouts._tmos, array_size);
  memcpy(_time, ebtimeouts._time, array_size);
}

EbTimeouts::EbTimeouts(unsigned crates, 
		       StreamParams::StreamType stream, 
		       Level::Type level) {
  const int array_size = Sequence::NumberOfServices*
    Sequence::NumberOfTypes*sizeof(_tmos[0]);
  memset(_tmos, 0, array_size);
  memset(_time, 0, array_size);

  if (stream == StreamParams::FrameWork) {
    _duration = framework_tmo;
  } else {
    _duration = occurence_tmo;
  }

  for (unsigned t=0; t<Sequence::NumberOfTypes; t++) {
    for (unsigned s=0; s<Sequence::NumberOfServices; s++) {
      Sequence::Type type = Sequence::Type(t);
      Sequence::Service service = Sequence::Service(s);
      int ds = delta(crates, stream, Level::Segment, type, service);
      int df = delta(crates, stream, Level::Fragment, type, service);
      int de;
      switch (level) {
      case Level::Control:
	de = delta(crates, stream, Level::Event, type, service);
	_tmos[type*Sequence::NumberOfServices+service] = 2*(ds+df)+de;
	_time[type*Sequence::NumberOfServices+service] = 4*(ds+df)+2*de;
	break;
      case Level::Event:
	de = delta(crates, stream, Level::Event, type, service);
	_tmos[type*Sequence::NumberOfServices+service] = ds+df;
	_time[type*Sequence::NumberOfServices+service] = 2*(ds+df)+de;
	break;
      case Level::Fragment:
	_tmos[type*Sequence::NumberOfServices+service] = ds;
	_time[type*Sequence::NumberOfServices+service] = ds+df;
	break;
      default:
	break;
      }
    }
  }
}

unsigned EbTimeouts::duration() const {
  return _duration;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {
  return _tmos[sequence->type()*Sequence::NumberOfServices+
	       sequence->service()];
}

int EbTimeouts::times(const Sequence* sequence) const {
  return _time[sequence->type()*Sequence::NumberOfServices+
	       sequence->service()];
}

void EbTimeouts::dump() const
{
  for (unsigned s=0; s<Sequence::NumberOfServices; s++) {
    printf("  %2d ->", s);
    for (unsigned t=0; t<Sequence::NumberOfTypes; t++) {
      printf(" %3d (%3d)", 
	     _tmos[t*Sequence::NumberOfServices+s],
	     _time[t*Sequence::NumberOfServices+s]);
    }
    printf("\n");
  }
}
