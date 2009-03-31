#include "EbC.hh"

#include "pds/xtc/CDatagram.hh"
#include "EbCountKey.hh"
#include "EbEvent.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

EbC::EbC(const Src& id,
	 const TypeId& ctns,
	 Level::Type level,
	 Inlet& inlet,
	 OutletWire& outlet,
	 int stream,
	 int ipaddress,
	 unsigned eventsize,
	 unsigned eventpooldepth,
	 VmonEb* vmoneb) :
  Eb(id, ctns, level, inlet, outlet,
     stream, ipaddress,
     eventsize, eventpooldepth, vmoneb),
  _keys( sizeof(EbCountKey), eventpooldepth)
{
}

EbC::~EbC()
{
}

EbEventBase* EbC::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (!depth)
    _postEvent(_pending.forward());

  CDatagram* datagram = new(&_datagrams) CDatagram(_ctns, _id);
  EbCountKey* key = new(&_keys) EbCountKey(const_cast<Datagram&>(datagram->datagram()).seq);
  return new(&_events) EbEvent(serverId, _clients, datagram, key);
}


