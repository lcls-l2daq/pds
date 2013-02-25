#include "ZcpEbS.hh"

#include "EbSequenceKey.hh"
#include "ZcpEbEvent.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/vmon/VmonEb.hh"

#include <string.h>

using namespace Pds;

ZcpEbS::ZcpEbS(const Src& id,
         const TypeId& ctns,
         Level::Type level,
         Inlet& inlet,
         OutletWire& outlet,
         int stream,
         int ipaddress,
         unsigned eventsize,
         unsigned eventpooldepth,
         int slowEb,
         VmonEb* vmoneb) :
  ZcpEb(id, ctns, level, inlet, outlet,
  stream, ipaddress,
  eventsize, eventpooldepth, slowEb, vmoneb),
  _keys( sizeof(EbSequenceKey), eventpooldepth )
{
  memset(_no_builds,0,sizeof(_no_builds));
}

ZcpEbS::~ZcpEbS()
{
}

void ZcpEbS::no_build(Sequence::Type type, unsigned mask)
{
  _no_builds[type] |= mask;
}

EbEventBase* ZcpEbS::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (!depth)
    _postEvent(_pending.forward());

  ZcpDatagram* datagram = new(&_datagrams) ZcpDatagram(_ctns, _id);
  EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()));
  return new(&_events) ZcpEbEvent(serverId, _clients, datagram, key);
}

EbBase::IsComplete ZcpEbS::_is_complete( EbEventBase* event,
           const EbBitMask& serverId)
{
  const Sequence& seq = event->key().sequence();
  if (_no_builds[seq.type()] & (1<<seq.service()))
    return NoBuild;
  return EbBase::_is_complete(event, serverId);
}

