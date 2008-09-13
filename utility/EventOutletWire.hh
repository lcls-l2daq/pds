#ifndef PDS_EVENTOUTLETWIRE_HH
#define PDS_EVENTOUTLETWIRE_HH

#include "OutletWire.hh"

#include "ToEb.hh"

namespace Pds {
class EventOutletWire : public OutletWire {
public:
  EventOutletWire(Outlet& outlet, 
		  int stream, 
		  int interface);
  virtual ~EventOutletWire() {}

  virtual InDatagram* forward(InDatagram* dg);
  virtual void bind(unsigned id, const Ins& node, int mcast) {_dst = node;}
  virtual void unbind(unsigned id) {_dst = Ins();}

private:
  ToEb _postman;
  Ins _dst;
};
}
#endif
