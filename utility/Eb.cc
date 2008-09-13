#include "Eb.hh"
#include "EbServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/SysClk.hh"
#include "pds/service/Client.hh"
#include "Inlet.hh"

//#define VERBOSE

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
       Level::Type level,
       Inlet& inlet,
       OutletWire& outlet,
       int stream,
       int ipaddress,
       unsigned eventsize,
       unsigned eventpooldepth,
       unsigned netbufdepth,
       const EbTimeouts& ebtimeouts,
#ifdef USE_VMON
       const VmonEb& vmoneb,
#endif
       const Ins* dstack) :
  EbBase(id, level, inlet, outlet, stream, ipaddress,
	 eventpooldepth, netbufdepth,
	 ebtimeouts,
#ifdef USE_VMON
	 vmoneb,
#endif
	 dstack),
  _datagrams(eventsize, eventpooldepth+1),
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

void Eb::_fixup( EbEventBase* event, const Src& client )
{
  EbEvent* ev = (EbEvent*)event;
  ev->fixup ( client, _dummy );
}

/*
** ++
**
**    This function is called for each contribution received from the
**    client it serves and thus forms the heart of event building. At
**    this point the contribution has already been copied into memory
**    at the addresses specified by the "payload" function described
**    above. However, the address and size of the payload is passed as
**    arguments to this function. In addition, the Datagram header is
**    copied into a separate buffer and passed as an argument ("header").
**    The client's IP address is also passed as an argument, but is not
**    used by this function. Recall, that the payload memory is allocated
**    one event ahead and although the server contains a pointer to the
**    event the contribution was copied into, the event specified by the
**    just arrived contribution may not correspond to the cached event.
**    So, first a search is made to find out what event the contribution
**    really belongs to. In the (hopefully, unlikely) event that the
**    cache is wrong, the right event must be found and the contribution
**    recopied into the new event. Statistics are kept on how often the
**    cache is hit or missed.
**    Once the event is located, the arrived contribution is than marked
**    off the list of contributions for which the event is waiting on
**    to complete ("event->consume"). This function returns a segment
**    descriptor if the arrived contribution was a fragment and more
**    fragments are still expected. If not the case and this is the last
**    contribution the event is waiting on ("event->remaining"), the built
**    datagram associated with the event is posted to its downstream
**    appliance and the event is returned to its free-list.
**    The function must return a non-zero value if the server's pending
**    function (from which this function is invoked) is to be re-invoked
**    without re-scheduling with any of this server's peers (see
**    "ManagedServers". This is the case when the server has a segment
**    in processio(i.e., its being built), because most likely the next
**    fragment is already in the receive queue. Not only does this save us
**    the overhead of going into the "ManagedServers's" select
**    unnecessarily, it allows the server to make a better guess as to what
**    buffer to allocate for the next received contribution.
**
** --
*/

int Eb::processIo(Server* serverGeneric)
{
#ifdef VERBOSE
  printf("Eb::processIo srvId %d\n",serverGeneric->id());
#endif
  EbServer* server = (EbServer*)serverGeneric;
  EbEvent*  event  = (EbEvent*)_event(server);
#ifdef VERBOSE
  printf("Eb::processIo event %p\n", event);
#endif
  if (!event) // Can't accept this contribution yet, may take it later
    return 0;

  EbBitMask serverId;
  serverId.setBit(server->id());
  int sizeofPayload  = server->fetch(event->payload(serverId), MSG_DONTWAIT);

#ifdef VERBOSE
  printf("Eb::processIo serverId %x payload_size 0x%x\n",
	 serverId.value(0),sizeofPayload);
#endif
  server->keepAlive();

  if(!sizeofPayload) {  // no payload
    if(event->deallocate(serverId).isZero()) { // this was the only contributor
      delete event->datagram();
      _eventpooldepth++;
      delete event;
    }
    return 1;
  }

  const EbPulseId* key = reinterpret_cast<const EbPulseId*>(server->key(EbKey::PulseId));
  if(!server->matches(event->key())) {
#ifdef VERBOSE
    printf("Eb::processIo key mismatch\n");
    event->key().print();
#endif
    if(event == event->forward()) {  // new event
      event->add(key, (EbEvent*)_seek(server));
#ifdef VERBOSE
      printf("Adding key to event\n");
      event->key().print();
#endif
    }
    else {
      _misses++;
      char* payload = event->payload(serverId);
      event->deallocate(serverId);
#ifdef VERBOSE
      printf("Eb::processIo missed event %p\n", event);
#endif
      if (!(event = (EbEvent*)_seek(server,serverId))) {  
#ifdef VERBOSE
	printf("Eb::processIo miss is fatal\n");
#endif
	return 0;
      }
#ifdef VERBOSE
      printf("Eb::processIo recopy to event %p\n", event);
#endif
      event->allocated().insert(serverId);
      event->recopy(payload, sizeofPayload, serverId);
    }
  }
  if(event->consume(server, sizeofPayload, serverId)) {  // expect more fragments?
    _segments++;
    return 1;
  }

  EbBitMask remaining = event->remaining(serverId);  // remove it from this event's list
  _hits++;

#ifdef VERBOSE
  printf("Eb::processIo remaining %x\n",remaining.value(0));
#endif
  //  If this client gave us more than one key re-arm those that could not be
  //  matched earlier
  unsigned keyMask = server->keyTypes();
  EbKey& ekey = event->key();
  if ((keyMask&=~ekey.types)) {
    if (keyMask & (1<<EbKey::PulseId))
      ekey.pulseId = *(EbPulseId*)server->key(EbKey::PulseId);
    if (keyMask & (1<<EbKey::EVRSequence))
      ekey.evrSequence = *(EbEvrSequence*)server->key(EbKey::EVRSequence);
    arm(remaining);
#ifdef VERBOSE
    printf("Eb::processIo adding keys %x\n",keyMask);
    ekey.print();
#endif
  }

  if (remaining.isZero())
    _postEvent(event);

  //  return 0;
  //  Remain armed to build future events.
  //  In general, events will be flushed by later complete events
  //  (or a full queue) rather than timing out.
  return 1;
}


EbEventBase* Eb::_new_event(const EbBitMask& serverId)
{
  CDatagram* datagram = new(&_datagrams) CDatagram(_header, _id);
  return new(&_events) EbEvent(serverId, _clients, datagram);
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
