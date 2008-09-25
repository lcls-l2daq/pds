#include "ZcpEbS.hh"

#include "EbSequenceKey.hh"
#include "ZcpEbEvent.hh"
#include "pds/xtc/ZcpDatagram.hh"

using namespace Pds;

ZcpEbS::ZcpEbS(const Src& id,
	       Level::Type level,
	       Inlet& inlet,
	       OutletWire& outlet,
	       int stream,
	       int ipaddress,
	       unsigned eventsize,
	       unsigned eventpooldepth) :
  ZcpEb(id, level, inlet, outlet,
	stream, ipaddress,
	eventsize, 
	eventpooldepth),
  _keys( sizeof(EbSequenceKey), eventpooldepth )
{
}

ZcpEbS::~ZcpEbS()
{
}

EbEventBase* ZcpEbS::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _events.numberofObjects()+_events.numberofFrees()-_events.numberofAllocs();

#ifdef USE_VMON
  if (_vmoneb.status() == VmonManager::Running) {
    _vmoneb.dginuse(depth);
  }
#endif

  if (!depth)
    _postEvent(_pending.forward());

  ZcpDatagram* datagram = new(&_datagrams) ZcpDatagram(_header, _id);
  EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()));
  return new(&_events) ZcpEbEvent(serverId, _clients, datagram, key);
}


