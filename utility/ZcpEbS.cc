#include "ZcpEbS.hh"

#include "EbSequenceKey.hh"
#include "ZcpEbEvent.hh"
#include "pds/xtc/ZcpDatagram.hh"

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
	       unsigned eventpooldepth) :
  ZcpEb(id, ctns, level, inlet, outlet,
	stream, ipaddress,
	eventsize, 
	eventpooldepth),
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

#ifdef USE_VMON
  if (_vmoneb.status() == VmonManager::Running) {
    _vmoneb.dginuse(depth);
  }
#endif

  if (depth==1 && _pending.forward()!=_pending.empty())
    _postEvent(_pending.forward());

  ZcpDatagram* datagram = new(&_datagrams) ZcpDatagram(_ctns, _id);
  EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()).seq);
  return new(&_events) ZcpEbEvent(serverId, _clients, datagram, key);
}

bool ZcpEbS::_is_complete( EbEventBase* event,
			   const EbBitMask& serverId)
{
  const Sequence& seq = event->key().sequence();
  if (_no_builds[seq.type()] & (1<<seq.service())) {
    event->remaining(event->remaining(serverId));
    return true;
  }
  return EbBase::_is_complete(event, serverId);
}

