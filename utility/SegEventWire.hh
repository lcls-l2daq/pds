#ifndef PDS_SEGEVENTWIRE_HH
#define PDS_SEGEVENTWIRE_HH

//#include "odfFragEventWire.hh"
#include "ToEventWire.hh"
#include "ToEb.hh"
#include "ReusePool.hh"

namespace Pds {
class SegEventWire : public ToEventWire {
public:
  SegEventWire(Outlet& outlet, 
	       int interface, 
	       unsigned slot,
	       unsigned crate);
  //	       AckHandler& ack_handler);

  virtual ~SegEventWire();

  // Implements OutletWire
  virtual InDatagram* forward(InDatagram* dg);

protected:
  ToEb      _postman;
};
}
#endif
