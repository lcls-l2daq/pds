/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**      non-inline functions for class "odfEb"
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 1,1998
**
**  Revision History:
**	None.
**
** --
*/

#include "ZcpEb.hh"

#include "pds/xtc/ZcpDatagram.hh"
#include "EbServer.hh"
#include "pds/service/SysClk.hh"
#include "pds/service/Client.hh"
#include "Inlet.hh"

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

ZcpEb::ZcpEb(const Src& id,
	     Level::Type level,
	     Inlet& inlet,
	     OutletWire& outlet,
	     int stream,
	     int ipaddress,
	     unsigned eventsize, // max size in # of IP datagrams
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
  _datagrams (sizeof(ZcpDatagram), eventpooldepth),
  _zfragments(sizeof(ZcpFragment), eventpooldepth*eventsize),
  _events    (sizeof(ZcpEbEvent) , eventpooldepth)
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

void ZcpEb::_fixup( EbEventBase* event, const Src& client )
{
  ZcpEbEvent* ev = (ZcpEbEvent*)event;
  ev->fixup(client, _dummy);
}

/*
**    In the zero-copy case, this function is simple.  Receive the
**    reference to the data and associate it with the proper event.
**    No guesswork is necessary here.  We can receive the event,
**    and look at its timestamp/key, before deciding which event to
**    place it in.
** --
*/

int ZcpEb::processIo(Server* serverGeneric)
{
#ifdef VERBOSE
  printf("ZcpEb::processIo srvId %d\n",serverGeneric->id());
#endif
  ZcpFragment* zfrag = new (&_zfragments) ZcpFragment;

  EbServer* server   = (EbServer*)serverGeneric;
  server->keepAlive();
  int sizeofPayload  = server->fetch(*zfrag, MSG_DONTWAIT);
  if(!sizeofPayload) {  // no payload
    delete zfrag;
    return 1;
  }

  EbBitMask serverId;
  serverId.setBit(server->id());

  ZcpEbEvent* zevent;
  {
    EbEventBase* event = _pending.forward();
    while(event != &_pending) {
      if (server->matches(event->key())) break;
      event = event->forward();
    }
    if (event == &_pending)
      event = _new_event(serverId);

    zevent = (ZcpEbEvent*)event;
  }
  EbBitMask remaining = zevent->add(server,
				    serverId,
				    zfrag);
  if ((remaining & serverId).isZero()) {
    _segments++;
    return 1;
  }

  _hits++;

#ifdef VERBOSE
  printf("ZcpEb::processIo remaining %x\n",remaining.value(0));
#endif

  //  If this client gave us more than one key try to merge events 
  //  which match its different key types,

  unsigned keyMask = server->keyTypes();
  EbKey& ekey = zevent->key();
  if ((keyMask&=~ekey.types)) {

    //  Add the additional keys to the event's key bank.

#ifdef VERBOSE
    printf("ZcpEb::processIo adding keys %x\n",keyMask);
    ekey.print();
#endif

    if (keyMask & (1<<EbKey::PulseId))
      ekey.pulseId = *(EbPulseId*)server->key(EbKey::PulseId);
    if (keyMask & (1<<EbKey::EVRSequence))
      ekey.evrSequence = *(EbEvrSequence*)server->key(EbKey::EVRSequence);

    EbEventBase* zb    = (EbEventBase*)zevent;
    EbEventBase* event = _pending.forward();
    EbEventBase* next;
    while(event != &_pending) {
      next = event->forward();
      if (event != zb && server->matches(event->key())) {
	zevent->merge( (ZcpEbEvent*)event->disconnect() );
      }
      event = next;
    }
  }

  if (remaining.isZero())
    _postEvent(zevent);

  //  Remain armed to build future events.
  //  In general, events will be flushed by later complete events
  //  (or a full queue) rather than timing out.
  return 1;
  }

EbEventBase* ZcpEb::_new_event(const EbBitMask& serverId)
{
  ZcpDatagram* datagram = new (&_datagrams) ZcpDatagram(_header, _id);

  return new(&_events) ZcpEbEvent(serverId, _clients, datagram);
}


/*
** ++
**
**   Destructor for class. Will remove each one of its server's from
**   its managed list by instantiating an iterator (see above). It
**   will then traverse the pending queue flushing any outstanding
**   events.
**
** --
*/

ZcpEb::~ZcpEb()
{
}

/*
** ++
**
** --
*/

#include <stdio.h>

void ZcpEb::_dump(int detail)
{
  printf(" %u Event buffers which manage datagrams of %u bytes each\n",
         _events.numberofObjects(), _datagrams.sizeofObject());
  printf(" Event buffers allocated/deallocated %u/%u\n",
         _events.numberofAllocs(), _events.numberofFrees());
  printf(" Datagrams allocated/deallocated %u/%u\n",
         _datagrams.numberofAllocs(), _datagrams.numberofFrees());
  printf(" ZFragments allocated/deallocated %u/%u\n",
         _zfragments.numberofAllocs(), _zfragments.numberofFrees());
}
