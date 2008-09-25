#ifndef Pds_Src_hh
#define Pds_Src_hh

#include "pds/collection/Level.hh"

namespace Pds {

  enum DetectorSys { BLD = 0xe0,
		     EVR = 0xf0 };

  class Src {
  public:
    //    Src(unsigned v) : _pid(v), _did(0) {}
    //    Src(unsigned pid, unsigned did) : _pid(pid), _did(did) {}
    Src() : _pid(-1UL), _did(-1UL) {}
    Src(Level::Type level   , unsigned node, 
	unsigned did) :
      _pid((level<<16) | node), 
      _did(did) {}
    Src(Level::Type level   , unsigned node, 
	DetectorSys detector, unsigned module) : 
      _pid((level   <<16) | node), 
      _did((detector<<16) | module) {}
    d_ULong did()   const { return _did; }
    d_ULong pid()   const { return _pid; }

    d_ULong level() const { return _pid>>16; }
    d_ULong node () const { return _pid&0xffff; }

    d_ULong detector() const { return _did>>16; }
    d_ULong module  () const { return _did&0xffff; }
    
    bool operator==(const Src& s) const { return _pid==s._pid && _did==s._did; }
  private:
    d_ULong _pid; // partition identity
    d_ULong _did; //  detector identity
  };

}
#endif
