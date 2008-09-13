#ifndef PDS_EVENTCONTROLWIRE_HH
#define PDS_EVENTCONTROLWIRE_HH

#include "OutletWire.hh"

#include "ToEb.hh"

namespace Pds {
class EventControlWire : public OutletWire {
public:
  EventControlWire(Outlet& outlet, 
		   int stream, 
		   int interface, 
		   unsigned id);
  virtual ~EventControlWire() {}

  virtual InDatagram* forward(InDatagram* dg);
  virtual void bind(unsigned id, const Ins& node, int mcast) {_dst = node;}
  virtual void unbind(unsigned id) {_dst = Ins();}

private:
  ToEb _postman;
  Ins _dst;
  unsigned _id; // Used by the elector mechanism
};
}
#endif
