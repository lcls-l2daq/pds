#include "ZcpEbC.hh"

#include "EbCountKey.hh"
#include "ZcpEbEvent.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

ZcpEbC::ZcpEbC(const Src& id,
         const TypeId& ctns,
         Level::Type level,
         Inlet& inlet,
         OutletWire& outlet,
         int stream,
         int ipaddress,
         unsigned eventsize,
         unsigned eventpooldepth,
         VmonEb* vmoneb) :
  ZcpEb(id, ctns, level, inlet, outlet,
  stream, ipaddress,
  eventsize, eventpooldepth, vmoneb),
  _keys( sizeof(EbCountKey), eventpooldepth )
{
}

ZcpEbC::~ZcpEbC()
{
}

EbEventBase* ZcpEbC::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (!depth)
    _postEvent(_pending.forward());

  ZcpDatagram* datagram = new(&_datagrams) ZcpDatagram(_ctns, _id);
  EbCountKey* key = new(&_keys) EbCountKey(const_cast<Datagram&>(datagram->datagram()));
  return new(&_events) ZcpEbEvent(serverId, _clients, datagram, key);
}


