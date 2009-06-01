#include "pds/utility/Eb.hh"
#include "pds/utility/EbServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/SysClk.hh"
#include "pds/service/Client.hh"
#include "pds/utility/Inlet.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

/*
** ++
**
**    Constructor for class. It requires the following arguments:
**    First,  a reference to an object which specifies the identity
**    the Event Builder is to assume "id", second, an enumeration of
**    the level (typically, "Fragment" or "Event") at which the Event
**    builder is to operate, third, "datagrams", a pointer to a pool
**    from which the memory to build the arrived events into will be
**    found, fourth, "tmo" an integer which specifies the length of
**    time (in 10 milli-second tics)
**    pointer specifying the task context in which all building activity
**    will occur. And, in addition, a pointer to the appliance into
**    which the assembled datagrams will be directed is also supplied.
**
** --
*/

Eb::Eb(const Src& id,
       const TypeId& ctns,
       Level::Type level,
       Inlet& inlet,
       OutletWire& outlet,
       int stream,
       int ipaddress,
       unsigned eventsize,
       unsigned eventpooldepth,
       VmonEb* vmoneb,
       const Ins* dstack) :
  EbBase(id, ctns, level, inlet, outlet, stream, ipaddress,
	 vmoneb, dstack),
  _datagrams(eventsize, eventpooldepth),
  _events(sizeof(EbEvent), eventpooldepth)
{
}

/*
** ++
**
**   This function is called back for each server which has not contributed
**   to the oldest pending event. The function is passed a pointer to the
**   server whose client is missing. The function will call back the
**   server's fixup function with the incomplete event and the appprpriate
**   dummy tagged container. This function will "patch" the event and return
**   the server's best guess as to whether the client proxied by the server
**   is still alive. If the client is labeled "dead", its server is
**   removed from the Event Builder and destroyed.
**
** --
*/

unsigned Eb::_fixup( EbEventBase* event, const Src& client, const EbBitMask& id )
{
  EbEvent* ev = (EbEvent*)event;
  return ev->fixup ( client, TypeId(TypeId::Any,0), id );
}

int Eb::processIo(Server* serverGeneric)
{
  EbServer* server = (EbServer*)serverGeneric;

  //  Find the next event waiting for a contribution from this server.
  //  If the event is better handled later (to avoid a copy) return 0.
  EbEvent*  event  = (EbEvent*)_event(server);
  if (!event) // Can't accept this contribution yet, may take it later
    return 0;

  //  Allocate space within this event-under-construction and receive the contribution.
  EbBitMask serverId;
  serverId.setBit(server->id());
  char* payload = event->payload(serverId);

  int sizeofPayload;
  if (_vmoneb && _vmoneb->time_fetch()) {
    unsigned begin = SysClk::sample();
    sizeofPayload  = server->fetch(payload, MSG_DONTWAIT);
    unsigned time  = SysClk::sample()-begin;
    _vmoneb->fetch_time(time);
  }
  else
    sizeofPayload  = server->fetch(payload, MSG_DONTWAIT);

  server->keepAlive();

  //  If there was an error on receive, remove the contribution from the event.
  //  Remove the event if this was the only contribution.
  if(sizeofPayload<0) {
    if(event->deallocate(serverId).isZero()) { // this was the only contributor
      delete event->finalize();
      delete event;
    }
    return 1;
  }

  //  The event key for the contribution may not match that of the event for two reasons:
  //  (1) it is the first contribution, and the event's key(s) are not yet set; or
  //  (2) the contribution came from a later event than expected.

  if(event == event->forward())    // case (1)
    _pending.insert(event);
  else if(!server->coincides(event->key())) {
    // case (2):  Remove the contribution from this event.  Now that we have the contribution's
    //            header, we can definitely search for the correct event-under-construction
    //            and copy the payload there.  If the correct event doesn't yet exist, it will
    //            be created, if possible.
    _misses++;
    event->deallocate(serverId);  // remove the contribution from this event
    event = (EbEvent*)_seek(server);
    if (event == (EbEvent*)_pending.empty()) {
      event = (EbEvent*)_new_event(serverId);
      _pending.insert(event);
    }

    event->allocated().insert(serverId);
    event->recopy(payload, sizeofPayload, serverId);
  }
  server->assign(event->key());

  //  Allow the event-under-construction to account for the added contribution
  if(sizeofPayload && event->consume(server, sizeofPayload, serverId)) {  // expect more fragments?
    _segments++;
    return 1;
  }

  _hits++;

  if (_is_complete(event, serverId))
    _postEvent(event);

  //  return 0;
  //  Remain armed to build future events.
  //  In general, events will be flushed by later complete events
  //  (or a full queue) rather than timing out.
  return 1;
}


Eb::~Eb()
{
}

void Eb::_dump(int detail)
{
  printf(" %u Event buffers which manage datagrams of %u bytes each\n",
         _events.numberofObjects(), _datagrams.sizeofObject());
  printf(" Event buffers allocated/deallocated %u/%u\n",
         _events.numberofAllocs(), _events.numberofFrees());
  printf(" Datagrams allocated/deallocated %u/%u\n",
         _datagrams.numberofAllocs(), _datagrams.numberofFrees());
}
