#include "EbC.hh"

#include "pds/xtc/CDatagram.hh"
#include "EbCountKey.hh"
#include "EbEvent.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

extern unsigned int nEbPrints;

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

EbEventBase* EbC::_new_event(const EbBitMask& serverId, char* payload, unsigned sizeofPayload)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (depth<=1 && _pending.forward()!=_pending.empty()) {
    if (nEbPrints)
      printf("EbC::new_event claiming buffer for srv %08x payload %d\n",
             serverId.value(), sizeofPayload);

    _post(_pending.forward());
  //    arm(_post(_pending.forward()));
  }

  EbEvent* event = 0;

  if (_datagrams.atHead()!=_datagrams.empty()) {
    CDatagram* datagram = new(&_datagrams) CDatagram(Datagram(_ctns, _id));
    EbCountKey* key = new(&_keys) EbCountKey(datagram->datagram());
    event = new(&_events) EbEvent(serverId, _clients, datagram, key);
    event->allocated().insert(serverId);
    event->recopy(payload, sizeofPayload, serverId);
  }

  return event;
}

EbEventBase* EbC::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.depth();

  if (_vmoneb) _vmoneb->depth(depth);

  if (depth<=1 && _pending.forward()!=_pending.empty()) {
    if (nEbPrints)
      printf("EbC::new_event claiming buffer for srv %08x\n",
             serverId.value());

    _post(_pending.forward());
  //    arm(_post(_pending.forward()));
  }

  EbEvent* event = 0;

  if (_datagrams.atHead()!=_datagrams.empty()) {
    CDatagram* datagram = new(&_datagrams) CDatagram(Datagram(_ctns, _id));
    EbCountKey* key = new(&_keys) EbCountKey(datagram->datagram());
    event = new(&_events) EbEvent(serverId, _clients, datagram, key);
  }

  return event;
}

//
//  The following two routines override the base class, so that
//  these event builders will always have a full "select mask";
//  i.e. fetch will be called for all pending IO.  This overrides
//  the base class behavior which only selects upon servers that
//  haven't yet contributed to the current event under construction.
//  This specialization is useful where the server data has
//  limited buffering (segment levels) and needs better realtime
//  response.
//
int EbC::poll()
{
  if(!ServerManager::poll()) return 0;
  if(active().isZero()) ServerManager::arm(managed());
  return 1;
}

int EbC::processIo(Server* srv)
{
  Eb::processIo(srv);
  return 1;
}
