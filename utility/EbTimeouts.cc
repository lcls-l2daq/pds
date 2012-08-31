#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

#ifdef BUILD_PRINCETON
static const int framework_tmo = 1000;
static const int occurence_tmo = 1000; // 200 ms
#else
static const int framework_tmo = 500;
static const int occurence_tmo = 200; // 200 ms
#endif


EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts) 
  : _duration(ebtimeouts._duration),
#ifdef BUILD_PRINCETON
    _tmos(60)
#else
    _tmos(2)
#endif
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
  case Level::Source : _tmos = 60; break;
  case Level::Segment: _tmos = 61; break;
  case Level::Event  : _tmos = 62; break;
  case Level::Control: _tmos = 63; break;
  default            : _tmos = 63; break;
#elif defined(BUILD_READOUT_GROUP)
  case Level::Source : _tmos = 4; break;
  case Level::Segment: _tmos = 5; break;
  case Level::Event  : _tmos = 6; break;
  case Level::Control: _tmos = 7; break;
  default            : _tmos = 7; break;
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

#if defined(BUILD_PRINCETON) || defined(BUILD_READOUT_GROUP)
  // No quick timeout for L1Accept
#else
  if (sequence && sequence->service()==TransitionId::L1Accept)
    return 1;
#endif

  return _tmos;
}

void EbTimeouts::dump() const
{
}
