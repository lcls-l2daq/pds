#include "ObserverEb.hh"

#include "pds/utility/EbServer.hh"

using namespace Pds;

//#define DBUG

ObserverEb::ObserverEb(const Src& id,
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
  EbSGroup(id, ctns, level, inlet, outlet,
           stream, ipaddress,
           eventsize, eventpooldepth, slowEb, vmoneb)
{
}

ObserverEb::~ObserverEb()
{
}

Server* ObserverEb::accept(Server* s)
{
  if (s->id()>=_last.size())
    _last.resize(s->id()+1);

  _last[s->id()]=0;

  return EbSGroup::accept(s);
}

//
//  Our guess for which event will get the next contribution
//  is based upon some assumption of the traffic shaping
//  algorithm.  Each new contribution will populate the next
//  event later than the previous contribution until the events
//  are complete.
//
EbEventBase* ObserverEb::_event( EbServer* server)
{
  EbEventBase *first=0;
  EbEventBase*& last=_last[server->id()];
  bool next=false;

  EbBitMask serverId;
  serverId.setBit(server->id());
  EbEventBase* event = _pending.forward();
  while( event != _pending.empty() ) {

    if(!(event->segments() & serverId).isZero() ||
       (event->allocated().remaining() & serverId).isZero()) {
      if (!first)      first=event;
      if (next) {
#ifdef DBUG
	printf("srv %d: event %p: last %p\n",
	       server->id(), event, last);
#endif
	event->allocated().insert(serverId);
	return (last=event);
      }
      if (event==last) next=true;
    }

    event = event->forward();
  }

  if (first) {
#ifdef DBUG
    printf("srv %d: first %p: last %p\n",
	   server->id(), first, last);
#endif
    first->allocated().insert(serverId);
    return (last=first);
  }
  
  event = _new_event(serverId);
#ifdef DBUG
  printf("srv %d: new %p: last %p\n",
	 server->id(), event, last);
#endif
  event->allocated().insert(serverId);
  return (last=event);
}

//
//  Our _event() guess was wrong.  Update with the correct event.
//  If no event found, default to taking the first pending event.
//
EbEventBase* ObserverEb::_seek(EbServer* server)
{
  EbEventBase* e = EbSGroup::_seek(server);
#ifdef DBUG
  printf("seek %d: event %p\n",server->id(),e);
#endif
  if (e) _last[server->id()]=e;
  else   _last[server->id()]=0;
  return e;
}
