#include "EbK.hh"

#include "EbEvent.hh"
#include "EbClockKey.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

extern unsigned nEbPrints;

EbK::EbK(const Src& id,
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
  _keys( sizeof(EbClockKey), eventpooldepth )
{
  memset(_no_builds,0,sizeof(_no_builds));
}

EbK::~EbK()
{
}

void EbK::no_build(Sequence::Type type, unsigned mask)
{
  _no_builds[type] |= mask;
}

//
//  Allocate a new datagram buffer and copy payload into it (from a previously allocated buffer)
//
EbEventBase* EbK::_new_event(const EbBitMask& serverId, char* payload, unsigned sizeofPayload)
{
  CDatagram* datagram = new(&_datagrams) CDatagram(Datagram(_ctns, _id));
  EbClockKey* key = new(&_keys) EbClockKey(datagram->datagram());
  EbEvent* event = new(&_events) EbEvent(serverId, _clients, datagram, key);
  event->allocated().insert(serverId);
  event->recopy(payload, sizeofPayload, serverId);

  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (depth<=1 && _pending.forward()!=_pending.empty()) {
    if (nEbPrints)
      printf("EbK::new_event claiming buffer for srv %08x payload %d\n",
             serverId.value(),sizeofPayload);

    _post(_pending.forward());
  //    arm(_post(_pending.forward()));
  }
  return event;
}

//
//  Allocate a new datagram buffer
//
EbEventBase* EbK::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (depth==1 && _pending.forward()!=_pending.empty()) { // keep one buffer for recopy possibility
    if (nEbPrints)
      printf("EbK::new_event claiming buffer for srv %08x\n",
             serverId.value());

    _post(_pending.forward());
  //    arm(_post(_pending.forward()));
  }

  CDatagram* datagram = new(&_datagrams) CDatagram(Datagram(_ctns, _id));
  EbClockKey* key = new(&_keys) EbClockKey(datagram->datagram());
  return new(&_events) EbEvent(serverId, _clients, datagram, key);
}

unsigned EbK::_fixup( EbEventBase* event, const Src& client, const EbBitMask& id )
{
  unsigned fixup = 0;
  const Sequence& seq = event->key().sequence();
  if (!(_no_builds[seq.type()] & (1<<seq.service()))) {
    EbEvent* ev = (EbEvent*)event;
    fixup = ev->fixup ( client, id );
  }
  return fixup;
}

EbBase::IsComplete EbK::_is_complete( EbEventBase* event,
                                      const EbBitMask& serverId)
{
  const Sequence& seq = event->key().sequence();
  if (_no_builds[seq.type()] & (1<<seq.service()))
    return NoBuild;
  return EbBase::_is_complete(event, serverId);
}
