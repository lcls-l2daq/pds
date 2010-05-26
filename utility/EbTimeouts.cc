#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

static const int framework_tmo = 500;  //250 ms
static const int occurence_tmo = 200; // 200 ms

EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts) 
  : _duration(ebtimeouts._duration),
    _tmos(2)
{
}

EbTimeouts::EbTimeouts(int stream, 
		       Level::Type level) {

  if (stream == StreamParams::FrameWork) {
    _duration = framework_tmo;
  } else {
    _duration = occurence_tmo;
  }
  
  switch(level) {
  case Level::Source : _tmos = 1; break;
  case Level::Segment: _tmos = 2; break;
  case Level::Event  : _tmos = 3; break;
  case Level::Control: _tmos = 4; break;
  default            : _tmos = 4; break;
  }
}

unsigned EbTimeouts::duration() const {
  return _duration;
}

unsigned EbTimeouts::duration(int s) {
  return (s == StreamParams::FrameWork) ? framework_tmo : occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {
  if (sequence && sequence->service()==TransitionId::L1Accept)
    return 1;
  return _tmos;
}

void EbTimeouts::dump() const
{
}
