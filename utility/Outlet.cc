#include "Outlet.hh"
#include "OutletWire.hh"
#include "pds/collection/CollectionManager.hh"

using namespace Pds;

// 
// The forward lists are initialised so that all data is forwarded to
// the next level
//

Outlet::Outlet(CollectionManager& cmgr) :
  _collection(cmgr),
  _wire(0)
{
  for (int i = 0; i< Sequence::NumberOfTypes; i++) _forward[i] = 0xffffffff;
}

void Outlet::plug(OutletWire& wire) {_wire = &wire;}
OutletWire* Outlet::wire() {return _wire;}

//
//  Collection messages are not allocated from any pool.  They
//  arrive in a fixed buffer.
// 

Transition* Outlet::transitions(Transition* tr) {
  Ins dst(tr->reply_to());
  _collection.ucast(*tr,dst);
  //  printf("outlet ucast transition %d to 0x%x:%d\n", 
  //	 tr->id(), dst.address(), dst.portId());
  return 0;
}

// Any incoming datagram are sent out on the wire (if that is
// appropriate). The wire can decide if the datagram must be deleted
// by the stream (by returning zero in forward()) or it can take care
// of the deletion (by returning Appliance::DontDelete in forward())

InDatagram* Outlet::events(InDatagram* datagram){
  const Datagram& dg = datagram->datagram();
  //  printf("outlet event service %x type %x forward 0x%x\n", 
  //	 dg.service(),dg.type(),_forward[dg.type()]);
  if ((1<<dg.service()) & _forward[dg.type()])
    return _wire->forward(datagram);
  else 
    return 0;
}

InDatagram* Outlet::occurrences(InDatagram* datagram){
  const Datagram& dg = datagram->datagram();
  if ((1<<dg.service()) & _forward[dg.type()]) 
    return _wire->forward(datagram);
  else 
    return 0;
}

InDatagram* Outlet::markers(InDatagram* datagram){
  const Datagram& dg = datagram->datagram();
  if ((1<<dg.service()) & _forward[dg.type()]) 
    return _wire->forward(datagram);
  else 
    return 0;
}

//
// Set which transitions should be forwarded/sink to the the next level
//

void Outlet::forward(OccurrenceId::Value id) 
{
  _forward[Sequence::Occurrence] |= (1 << id);
}

void Outlet::forward(EventId::Value id)
{
  _forward[Sequence::Event] |= (1 << id);
}

void Outlet::forward(MarkerId::Value id)
{
  _forward[Sequence::Marker] |= (1 << id);
}

void Outlet::forward(Sequence::Type type, unsigned mask)
{
  _forward[type] = mask;
}

void Outlet::sink(OccurrenceId::Value id) 
{
  _forward[Sequence::Occurrence] &= ~(1 << id);
}

void Outlet::sink(EventId::Value id)
{
  _forward[Sequence::Event] &= ~(1 << id);
}

void Outlet::sink(MarkerId::Value id)
{
  _forward[Sequence::Marker] &= ~(1 << id);
}

void Outlet::sink(Sequence::Type type, unsigned mask)
{
  _forward[type] = ~mask;
}

unsigned Outlet::forward(Sequence::Type type) const
{
  return _forward[type];
}
