#include "SegEventWire.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
//#include "AckRequest.hh"
#include "Mtu.hh"

static const unsigned MaxDatagrams = 4;

static const unsigned Clients = 0;

using namespace Pds;

SegEventWire::SegEventWire(Outlet& outlet, 
			   int interface, 
			   unsigned slot,
			   unsigned crate ) :
  //			   AckHandler& ack_handler) :
//  ToEventWire(outlet, slot, Clients, crate, ack_handler),
  ToEventWire(outlet, slot, Clients, crate),
  // max of sizeof(EbBitMaskArrayXtc) and sizeof(Datagram),
  // max number of additions (trailers/headers) to a transmitted datagram
  _postman(interface, Mtu::Size, MaxDatagrams)

{}

SegEventWire::~SegEventWire() {}


InDatagram* SegEventWire::forward(InDatagram* datagram)
{
  if (isempty()) return 0;

  //  AckRequest*    ack;
  OutletWireIns* dst;

  //  dst = prepareEvent(datagram, ack);
  dst = prepareEvent(datagram->datagram());

  int result = datagram->send(_postman, dst->ins());
  if (result) {
    //    if (ack) ack->status(result);
    _log(datagram->datagram(), result);
  }
  return 0;
}
