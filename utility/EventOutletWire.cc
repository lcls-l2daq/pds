#include "EventOutletWire.hh"
#include "Mtu.hh"
#include "pds/xtc/InDatagram.hh"

static const unsigned MaxDatagrams = 32;

using namespace Pds;

EventOutletWire::EventOutletWire(Outlet& outlet, 
				 int stream, 
				 int interface) :
  OutletWire(outlet),
  _postman(interface, Mtu::Size, MaxDatagrams),
  _dst()
{}

InDatagram* EventOutletWire::forward(InDatagram* dg)
{
  int result = dg->send(_postman, _dst);
  if (result) _log(dg->datagram(), result);

  return 0;
}
