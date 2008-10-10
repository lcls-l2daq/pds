#ifndef Pds_Src_hh
#define Pds_Src_hh

#include "pds/collection/Node.hh"
#include "pds/service/ODMGTypes.hh"

namespace Pds {

  enum DetectorSys { BLD = 0xe0,
		     EVR = 0xf0 };

  class Src {
  public:
    //    Src(unsigned v) : _phy(v), _log(0) {}
    //    Src(unsigned pid, unsigned did) : _phy(pid), _log(did) {}
    Src() : _log(-1UL), _phy(-1UL) {}
    Src(const Node& hdr) : _log((hdr.level()<<24) | hdr.pid()&0x00ffffff),
			   _phy(hdr.ip()) {}
    Src(const Node& hdr,
	unsigned deviceId) : _log((hdr.level()<<24) | hdr.pid()&0x00ffffff),
			     _phy(deviceId) {}

    d_ULong log()   const { return _log; }
    d_ULong phy()   const { return _phy; }

    d_ULong level() const { return _log>>24; }
    d_ULong pid  () const { return _log&0xffffff; }

    d_ULong detector() const { return _phy>>16; }
    d_ULong module  () const { return _phy&0xffff; }
    
    bool operator==(const Src& s) const { return _phy==s._phy && _log==s._log; }
  private:
    d_ULong _log; //  logical identifier
    d_ULong _phy; // physical identifier
  };

}
#endif
