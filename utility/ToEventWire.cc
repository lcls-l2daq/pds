#include "ToEventWire.hh"

#include "Mtu.hh"
#include "Transition.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"

static const unsigned MaxDatagrams = 32;

using namespace Pds;

ToEventWire::ToEventWire(Outlet& outlet,
			 CollectionManager& collection,
			 int interface) :
  OutletWire(outlet),
  _collection(collection),
  _postman(interface, Mtu::Size, MaxDatagrams)
{
}

ToEventWire::~ToEventWire()
{
}

Transition* ToEventWire::forward(Transition* tr)
{
  Ins dst(tr->reply_to());
  _collection.ucast(*tr,dst);
  return 0;
}

InDatagram* ToEventWire::forward(InDatagram* dg)
{
  OutletWireIns* dst = _nodes.lookup(dg->datagram());
  int result = dg->send(_postman, dst->ins());
  if (result) _log(dg->datagram(), result);

  return 0;
}

void ToEventWire::bind(unsigned id, const Ins& node) 
{
  _nodes.insert(id, node);
}

void ToEventWire::unbind(unsigned id) 
{
  _nodes.remove(id);
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
