#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

#ifdef BUILD_PRINCETON
static const int framework_tmo = 1000;
#else
static const int framework_tmo = 500;
#endif

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

#ifdef BUILD_PRINCETON
  case Level::Source : _tmos = 400; break;
  case Level::Segment: _tmos = 401; break;
  case Level::Event  : _tmos = 402; break;
  case Level::Control: _tmos = 403; break;
  default            : _tmos = 403; break;
#else
  case Level::Source : _tmos = 1; break;
  case Level::Segment: _tmos = 2; break;
  case Level::Event  : _tmos = 3; break;
  case Level::Control: _tmos = 4; break;
  default            : _tmos = 4; break;
#endif

  }
}

unsigned EbTimeouts::duration() const {
  return _duration;
}

unsigned EbTimeouts::duration(int s) {
  return (s == StreamParams::FrameWork) ? framework_tmo : occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {

#ifdef BUILD_PRINCETON
  // No special timeout for L1Accept
#else
  if (sequence && sequence->service()==TransitionId::L1Accept)
    return 1;
#endif

  return _tmos;
}

void EbTimeouts::dump() const
{
}
