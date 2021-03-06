#include "ToEbWire.hh"
#include "Mtu.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
#include "Appliance.hh"

using namespace Pds;

ToEbWire::ToEbWire(Outlet& outlet, 
		   InletWire& inlet,
		   ToEb&   postman) :
  OutletWire(outlet),
  _inlet(inlet),
  _postman(postman)
{}

Transition* ToEbWire::forward(Transition* dg)
{
  _inlet.post(*dg);
  return (Transition*)Appliance::DontDelete;  // inserted out-of-band "locally" - still in use
}

Occurrence* ToEbWire::forward(Occurrence* dg)
{
  _inlet.post(*dg);
  return (Occurrence*)Appliance::DontDelete;  // inserted out-of-band "locally" - still in use
}

InDatagram* ToEbWire::forward(InDatagram* dg)
{
  const Datagram& datagram = dg->datagram();
  /*
  {
    unsigned* d = (unsigned*)&datagram;
    printf("ToEbWire:: %08x/%08x %08x/%08x %08x\n"
	   "           %08x %08x %08x %08x %08x\n",
	   d[0], d[1], d[2], d[3], d[4],
	   d[5], d[6], d[7], d[8], d[9]);
  }
  */
  if (!datagram.seq.isEvent()) {
    _inlet.post(*dg);
  }
  else {
    int result = dg->send(_postman);
    if (result) _log(dg->datagram(), result);
  }
  return (InDatagram*)Appliance::DontDelete;  // inserted out-of-band "locally" - still in use 
}
