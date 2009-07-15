#include "pds/xtc/EnableEnv.hh"

#include "pdsdata/xtc/ClockTime.hh"

using namespace Pds;

static const unsigned event_bit  = (1UL<<31);
static const unsigned event_mask = (1UL<<31)-1;

EnableEnv::EnableEnv(unsigned n        ) : 
  Env(n | event_bit) {}
EnableEnv::EnableEnv(const ClockTime& d) : 
  Env((d.seconds()*1000 + d.nanoseconds()/1000000) & event_mask) {}

bool     EnableEnv::timer   () const { return !(value() & event_bit); }
unsigned EnableEnv::duration() const { return value(); }
unsigned EnableEnv::events  () const { return timer() ? 0 : value() & event_mask; }
