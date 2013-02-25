#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

//#ifdef BUILD_PRINCETON
//static const int framework_tmo = 1000;
//static const int occurence_tmo = 1000; // 200 ms
//#else
//static const int framework_tmo = 500;
//static const int occurence_tmo = 200; // 200 ms
//#endif
static const int framework_tmo = 500;
static const int occurence_tmo = 200; // 200 ms


EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts, int iSlowEb)
  : _duration(ebtimeouts._duration),
//#ifdef BUILD_PRINCETON
//    _tmos(60)
//#else
//    _tmos(2)
//#endif
  _tmos(iSlowEb > 0 ? 120: 2)
{
}

EbTimeouts::EbTimeouts(int stream,
           Level::Type level,
           int iSlowEb)
{
  if (stream == StreamParams::FrameWork) {
    _duration = framework_tmo;
  } else {
    _duration = occurence_tmo;
  }

  switch(level) {
  case Level::Source : _tmos = (iSlowEb > 0? 120:4); break;
  case Level::Segment: _tmos = (iSlowEb > 0? 122:5); break;
  case Level::Event  : _tmos = (iSlowEb > 0? 124:6); break;
  case Level::Control: _tmos = (iSlowEb > 0? 126:7); break;
  default            : _tmos = (iSlowEb > 0? 126:7); break;
  }

//  switch(level) {
//#ifdef BUILD_PRINCETON
//    case Level::Source : _tmos = 60; break;
//    case Level::Segment: _tmos = 61; break;
//    case Level::Event  : _tmos = 62; break;
//    case Level::Control: _tmos = 63; break;
//    default            : _tmos = 63; break;
//#elif defined(BUILD_READOUT_GROUP)
//    case Level::Source : _tmos = 4; break;
//    case Level::Segment: _tmos = 5; break;
//    case Level::Event  : _tmos = 6; break;
//    case Level::Control: _tmos = 7; break;
//    default            : _tmos = 7; break;
//#else
//    case Level::Source : _tmos = 1; break;
//    case Level::Segment: _tmos = 2; break;
//    case Level::Event  : _tmos = 3; break;
//    case Level::Control: _tmos = 4; break;
//    default            : _tmos = 4; break;
//#endif
//  }

  printf("EbTimeouts::EbTimeouts(stream = %d, level = %s, iSlowEb = %d) tmo = %d\n",
    stream, Level::name(level), iSlowEb, _tmos);

}

unsigned EbTimeouts::duration() const {
  return _duration;
}

unsigned EbTimeouts::duration(int s) {
  return (s == StreamParams::FrameWork) ? framework_tmo : occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {

  printf("EbTimeouts::timeouts() tmo = %d\n", _tmos);

  return _tmos;
//#if defined(BUILD_PRINCETON) || defined(BUILD_READOUT_GROUP)
//  // No quick timeout for L1Accept
//#else
//  if (sequence && sequence->service()==TransitionId::L1Accept)
//    return 1;
//#endif

//  return _tmos;
}

void EbTimeouts::dump() const
{
}
