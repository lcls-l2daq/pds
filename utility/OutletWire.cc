#include <string.h>

#include "OutletWire.hh"
#include "Outlet.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/service/Ins.hh"

using namespace Pds;

OutletWire::OutletWire(Outlet& outlet)
{
  outlet.plug(*this);
}

OutletWire::~OutletWire() {}

void OutletWire::_log(const Datagram& dg, int result)
{
  printf("*** OutletWire send seq %08x %08x, extent %08x error: %s\n",
         dg.highAll(), dg.low(), dg.xtc.tag.extent(), strerror(result));
}


void OutletWire::_log(const Datagram& dg, const Ins& ins, int result)
{
  printf("*** OutletWire send seq %08x %08x, extent %08x, addr %08x, port %d, error: %s\n",
         dg.highAll(), dg.low(), dg.xtc.tag.extent(), 
	 ins.address(), ins.portId(),
	 strerror(result));
}
