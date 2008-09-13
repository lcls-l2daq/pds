#include "EventControlWire.hh"
#include "Mtu.hh"
#include "pds/xtc/InDatagram.hh"

static const unsigned MaxDatagrams = 32;

using namespace Pds;

EventControlWire::EventControlWire(Outlet& outlet, 
				   int stream, 
				   int interface, 
				   unsigned id) :
  OutletWire(outlet),
  _postman(interface, Mtu::Size, MaxDatagrams),
  _dst(),
  _id(id)
{}

InDatagram* EventControlWire::forward(InDatagram* dg)
{
  int result = dg->send(_postman, _dst);
  if (result) _log(dg->datagram(), result);

  return 0;
}
