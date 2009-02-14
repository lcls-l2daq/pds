#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

static const int framework_tmo = 500; // 10 ms
static const int occurence_tmo = 200; // 200 ms

EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts) 
  : _duration(ebtimeouts._duration)
{
  const int array_size = TransitionId::NumberOf*
    Sequence::NumberOfTypes*sizeof(_tmos[0]);
  memcpy(_tmos, ebtimeouts._tmos, array_size);
}

EbTimeouts::EbTimeouts(int stream, 
		       Level::Type level) {
  const int array_size = TransitionId::NumberOf*
    Sequence::NumberOfTypes*sizeof(_tmos[0]);
  memset(_tmos, 0, array_size);

  if (stream == StreamParams::FrameWork) {
    _duration = framework_tmo;
  } else {
    _duration = occurence_tmo;
  }

  for (unsigned t=0; t<Sequence::NumberOfTypes; t++) {
    for (unsigned s=0; s<TransitionId::NumberOf; s++) {
      Sequence::Type type = Sequence::Type(t);
      TransitionId::Value service = TransitionId::Value(s);
      switch (level) {
      case Level::Control:
	_tmos[type*TransitionId::NumberOf+service] = 2;
	break;
      case Level::Segment:
	_tmos[type*TransitionId::NumberOf+service] = 2;
	break;
      case Level::Event:
	_tmos[type*TransitionId::NumberOf+service] = 2;
	break;
      case Level::Recorder:
	_tmos[type*TransitionId::NumberOf+service] = 2;
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

unsigned EbTimeouts::duration(int s) {
  if (s == StreamParams::FrameWork)
    return framework_tmo;
  return occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {
  //  return _tmos[sequence->type()*TransitionId::NumberOf+
  //  	       sequence->service()];
  return _tmos[0];
}

void EbTimeouts::dump() const
{
  for (unsigned s=0; s<TransitionId::NumberOf; s++) {
    printf("  %2d ->", s);
    for (unsigned t=0; t<Sequence::NumberOfTypes; t++) {
      printf(" %3d", 
	     _tmos[t*TransitionId::NumberOf+s]);
    }
    printf("\n");
  }
}
