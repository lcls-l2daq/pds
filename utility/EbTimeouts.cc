#include <stdio.h>
#include <string.h>

#include "EbTimeouts.hh"

using namespace Pds;

static const int framework_tmo = 250;  //250 ms
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
  
  _tmos = 2;
}

unsigned EbTimeouts::duration() const {
  return _duration;
}

unsigned EbTimeouts::duration(int s) {
  return (s == StreamParams::FrameWork) ? framework_tmo : occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {
  //  return _tmos[sequence->type()*TransitionId::NumberOf+
  //  	       sequence->service()];
  return _tmos;
}

void EbTimeouts::dump() const
{
}
