#include "ToEventWire.hh"

//#include "AckHandler.hh"
#include "Mtu.hh"
#include "EbHeaderTC.hh"
#include "Event.hh"

using namespace Pds;

ToEventWire::ToEventWire(Outlet& outlet, 
			 unsigned slot,
			 unsigned clients,
			 const Src& id) :
			 //			       AckHandler& ack_handler) :
  OutletWire(outlet),
  _mcast((unsigned)-1, Ins()),
  //  _ack_handler(ack_handler),
  _id(id)
{
}

ToEventWire::~ToEventWire()
{
}

void ToEventWire::bind(unsigned id, const Ins& node, int mcast) 
{
  _nodes.insert(id, node);
  _mcast.replace(Ins(mcast, node.portId()));
}

void ToEventWire::unbind(unsigned id) 
{
  _nodes.remove(id);
}

//#define BatchFragId  TypeIdPrimary(EbBatchTC::FragIdentity)

OutletWireIns* ToEventWire::prepareEvent(const Datagram& datagram)
  //					 AckRequest*& ack) 
{
  OutletWireIns* dst = _nodes.lookup(datagram.low() >> 7);

  //  A non-batched L1Accept
  if (!datagram.notEvent()) {
    //    ack = _ack_handler.queue(*datagram, dst->id());
    return dst;
  }

  //  ack = 0;
  return &_mcast;
}

void ToEventWire::dump(int detail)
{
  //  printf("Data flow throttle:\n");
  //  FlowThrottle::connect()->dump(detail);

  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      printf(" Event id %i, port %i, address 0x%x\n",
	     current->id(),
	     current->ins().portId(),
	     current->ins().address());
      //      _ack_handler.dump(i, detail);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}

void ToEventWire::dumpHistograms(unsigned tag, const char* path)
{
  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      //      _ack_handler.dumpHistograms(i, tag, path);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}

void ToEventWire::resetHistograms()
{
  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      //      _ack_handler.resetHistograms(i);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}
