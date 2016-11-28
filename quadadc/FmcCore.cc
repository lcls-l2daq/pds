#include "pds/quadadc/FmcCore.hh"

using namespace Pds::QuadAdc;

bool FmcCore::present() const
{ return (_detect&1)==0; }

bool FmcCore::powerGood() const
{ return _detect&2; }

void FmcCore::selectClock(unsigned i)
{ _clock_select = i; }

double FmcCore::clockRate() const
{ return double(_clock_count)/8192.*125.e6; }
