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
#include "pds/vmon/VmonEb.hh"

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

ZcpEb::ZcpEb(const Src& id,
	     const TypeId& ctns,
	     Level::Type level,
	     Inlet& inlet,
	     OutletWire& outlet,
	     int stream,
	     int ipaddress,
	     unsigned eventsize, // max size in # of IP datagrams
	     unsigned eventpooldepth,
	     VmonEb* vmoneb,
	     const Ins* dstack) :
  EbBase(id, ctns, level, inlet, outlet, stream, ipaddress,
	 vmoneb, dstack),
  _datagrams (sizeof(ZcpDatagram), eventpooldepth),
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

unsigned ZcpEb::_fixup( EbEventBase* event, const Src& client, const EbBitMask& id )
{
  ZcpEbEvent* ev = (ZcpEbEvent*)event;
  return ev->fixup(client, id, TypeId(TypeId::Any,0), _zfragment);
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
  EbServer* server   = (EbServer*)serverGeneric;
  server->keepAlive();

  int sizeofPayload;
  if (_vmoneb && _vmoneb->time_fetch()) {
    unsigned begin = SysClk::sample();
    sizeofPayload  = server->fetch(_zfragment, MSG_DONTWAIT);
    unsigned time  = SysClk::sample()-begin;
    _vmoneb->fetch_time(time);
  }
  else
    sizeofPayload  = server->fetch(_zfragment, MSG_DONTWAIT);

  if(sizeofPayload<0) {  // no payload
    printf("ZcpEb::processIo error in fetch more/len/off %c/%x/%x\n",
	   server->more() ? 't':'f', 
	   server->length(), 
	   server->offset());
    _zfragment.flush();
    return 1;
  }

#ifdef VERBOSE
  printf("ZcpEb::processIo fetch frag size %d\n",_zfragment.size());
#endif
  EbBitMask serverId;
  serverId.setBit(server->id());

  //
  //  Search for an event with a matching key
  //  If no match is found, append a new event onto the pending queue 
  //
  ZcpEbEvent* zevent;
  {
    EbEventBase* event = _pending.forward();
    while(event != _pending.empty()) {
      if (server->coincides(event->key())) break;
      event = event->forward();
    }
    if (event == _pending.empty()) {
      event = _new_event(serverId);
      _pending.insert(event);
    }

    zevent = (ZcpEbEvent*)event;
  }
  server->assign(zevent->key());
  if (sizeofPayload==0)
    ;
  else if (zevent->consume(server,
			   serverId,
			   _zfragment)) {
    _segments++;
    return 1;
  }

  _hits++;

  if (_is_complete(zevent, serverId))
    _postEvent(zevent);

  //  Remain armed to build future events.
  //  In general, events will be flushed by later complete events
  //  (or a full queue) rather than timing out.
  return 1;
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
}
