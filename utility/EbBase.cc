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

#include "pds/utility/EbBase.hh"
#include "pds/utility/EbServer.hh"
#include "pds/utility/EbTimeouts.hh"
#include "pds/utility/Inlet.hh"
#include "pds/vmon/VmonEb.hh"

#include "pds/service/SysClk.hh"
#include "pds/service/Client.hh"

#include <string.h>

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

static const int TaskPriority = 60;
static const char* TaskLevelName[Level::NumberOfLevels+1] = {
  "oEbCtr", "oEbSrc", "oEbSeg", "oEbEvt", "oEbRec", "oEbObs"
};
static const char* TaskName(Level::Type level, int stream, Inlet& inlet)
{
  static char name[64];
  sprintf(name, "%s%d_%p", TaskLevelName[level], stream, &inlet);
  return name;
}


EbBase::EbBase(const Src& id,
	       const TypeId& ctns,
	       Level::Type level,
	       Inlet& inlet,
	       OutletWire& outlet,
	       int stream,
	       int ipaddress,
	       VmonEb* vmoneb,
	       const Ins* dstack) :
  InletWireServer(inlet, outlet, ipaddress, stream, 
		  TaskPriority-stream, TaskName(level, stream, inlet),
		  EbTimeouts::duration(stream)),
  _ebtimeouts(stream,level),
  _output(inlet),
  _id(id),
  _ctns(ctns),
  _hits(0),
  _segments(0),
  _misses(0),
  _discards(0),
  _ack(0),
  _vmoneb(vmoneb)
{
  if (dstack) {
    const unsigned PayloadSize = 0;
    _ack = new Client(sizeof(Datagram), PayloadSize, 
			 *dstack, Ins(ipaddress));
  }
}

/*
** ++
**
**    This function is called back whenever a client comes into existence.
**    Its passed a dense number which uniquely identifies the client and
**    the IP address of the client itself. This function will first register
**    the client with the database representing the set of clients currently
**    known to the builder. Second, it will allocate a context block to
**    perform deletion of the server created next. Third, it will create a
**    network server to handle any incoming contributions from this specific
**    client. And last, it will register the created server with the database
**    representing the set of servers currently managed by the builder.
**
** --
*/

Server* EbBase::accept(Server* srv)
{
  EbBitMask clients = managed();
  unsigned id = 0;
  while( clients.LSBnotZero() ) {
    clients >>= 1;
    id++;
  }
  srv->id(id);

  EbServer* accepted = (EbServer*)srv;
  if (accepted->isValued())
    _valued_clients.setBit(id);

  manage(accepted);
  ServerManager::arm(accepted);
  _clients = managed();

  return accepted;
}

/*
** ++
**
**   This function will remove and delete the server specified
**   by theinput argument... The function returns no value.
**
** --
*/

void EbBase::_remove(EbServer* server)
{
  delete unmanage(server);
}

/*
** ++
**
**   This function will remove and delete the server associated with
**   a particular client. The client is specified by the argument
**   ("client") passed to the function. The function returns no value.
**
** --
*/

void EbBase::remove(unsigned id)
{
  EbServer* srv = (EbServer*)server(id);
  if (srv->isValued())
    _valued_clients.clearBit(id);
  _remove(srv);
  _clients = managed();
}

/*
** ++
**
**   Called to flush pending events after eb is disconnected and
**   before dtor is called.
**
** --
*/

void EbBase::flush()
  {
    EbEventBase* event = _pending.forward();
    EbEventBase* empty = _pending.empty();
    while( event != empty ) {
      _post(event);
      event = _pending.forward();
    }
  }

/*
** ++
**
**
** --
*/

EbBitMask EbBase::_armMask(EbEventBase* current, EbEventBase* empty)
{
  EbBitMask participants = managed();
  _clients = participants;

  if (current == empty) return  participants;

  do{
    participants &= current->remaining();
  }
  while ((current = current->forward()) != empty);

  return participants;
}

#define PAUSE     (1 << Sequence::Service20)
#define DISABLE   (1 << Sequence::Service28)

void EbBase::_post(EbEventBase* event)
{
  InDatagram* indatagram = event->finalize();
  Datagram*   datagram   = const_cast<Datagram*>(&indatagram->datagram());
  EbBitMask   remaining  = event->remaining();

  EbBitMask value((event->allocated().remaining() |
		   event->segments()) &
		  _valued_clients);
#ifdef VERBOSE
  printf("(%p) %08x/%08x remaining %08x value %08x payload %d\n",
    	 this, datagram->seq.service(),datagram->seq.stamp().fiducials(),
  	 remaining.value(0),value.value(0),datagram->xtc.sizeofPayload());
#endif
  if (value.isZero()) {  // sink
    delete indatagram;
    delete event;
    return;
  }

#ifdef VERBOSE
  printf("EbBase::_post %p  remaining %x\n",
	 event, remaining.value(0));
#endif

  if(remaining.isNotZero()) {

    char buff[80];
    sprintf(buff,"EbBase::_post fixup seq %08x remaining ",
	    datagram->seq.stamp().fiducials());
    remaining.write(&buff[strlen(buff)]);

    // statistics
    EbBitMask id(EbBitMask::ONE);
    unsigned dmg=0;
    for(unsigned i=0; !remaining.isZero(); i++, id <<= 1) {
      if ( !(remaining & id).isZero() ) {
	if (_vmoneb) _vmoneb->fixup(i);
	EbServer* srv = (EbServer*)server(i);
	srv->fixup();
	dmg |= _fixup(event, srv->client(), id);
	remaining &= ~id;
      }
    }
    printf("%s dmg 0x%x\n",buff,dmg);

    datagram->xtc.damage.increase(dmg);
  }

  if (_vmoneb) {
    ClockTime clock(indatagram->datagram().seq.clock());
    _vmoneb->post_size(indatagram->datagram().xtc.extent);
    unsigned begin = SysClk::sample();
    _output.post(indatagram);
    unsigned time  = SysClk::sample()-begin;
    _vmoneb->post_time(time);
    _vmoneb->fixup(-1);
    _vmoneb->update(clock);
  } else {
    _output.post(indatagram);
  }

  // Send acks on L1Accept, Pause and Disable; the latter two to flush the last
  // L1Accept.  Since an ack wasn't requested for Pause or Disable, it will be
  // ignored by the fragment level (ignored counter will increment).
  Client* ack = _ack;
  //  if (ack && (!datagram->notEvent() ||
  //              ((1 << datagram->service()) & (PAUSE | DISABLE))))
  if (ack && (datagram->seq.isEvent()))
    ack->send((char*)datagram, (char*) 0, 0);

  delete event;
}

#include "pds/utility/EbEvent.hh"

EbBitMask EbBase::_postEvent(EbEventBase* complete)
{
//
//  The network stack can no longer be trusted to deliver packets in
//  chronological order (consequence of SMP).
//
   EbEventBase* event = _pending.forward();
   EbEventBase* empty = _pending.empty();
   
   if (event != empty &&
       event != complete) 
     printf("%x/%x(%x)... pushed by %x/%x\n",
	    event->datagram()->seq.service(),
	    event->datagram()->seq.stamp().fiducials(),
	    event->remaining().value(0),
	    complete->datagram()->seq.service(),
	    complete->datagram()->seq.stamp().fiducials());

   while( event != empty ) {
     _post(event);
     if (event == complete) break;
     event = _pending.forward();
   }

  return managed();
}

/*
** ++
**
**   This function is called periodically by the timer associated with
**   the class. Its function is to ferret out those events which are in
**   the process of completing, yet are stalled FROM completing because
**   they are waiting for one (or more) contributions, which for some
**   unknown reason will NEVER arrive. The idea is to look for the oldest
**   pending completion event which has expired (i.e., its seen this function
**   "tics" times). If an expired event is found, a message is composed and
**   send which will satisfy the event's completion. Note: that the message
**   has its damage field marked as "timeout", allowing the completed event
**   to be aware of the reason it was completed. Note also, that the message
**   is sent to all those servers which still have a read outstanding on
**   this event.
**
** --
*/

int EbBase::processTmo()
  {
  EbEventBase* event = _pending.forward();
  EbEventBase* empty = _pending.empty();
  if (event != empty) {
    if (event->timeouts(_ebtimeouts) > 0) {
      //  mw- Recalculate enable mask - could be done faster (not redone)
      ServerManager::arm(_armMask(event,empty));
    } else {
#ifdef VERBOSE
      InDatagram* indatagram   = event->finalize();
      const Datagram* datagram = &indatagram->datagram();
      EbBitMask value(event->allocated().remaining() & _valued_clients);
      if (!value.isZero())
	printf("EbBase::processTmo seq %x/%x  remaining %08x\n",
	       datagram->seq.service(), datagram->seq.stamp().fiducials(),
	       event->remaining().value());
#endif
      ServerManager::arm(_postEvent(event));
    }
  } else {
    ServerManager::arm(managed());
  }
  return 1;
  }

/*
** ++
**
**
** --
*/

int EbBase::poll()
  {
  if(!ServerManager::poll()) return 0;

  if(active().isZero()) ServerManager::arm(managed());

  return 1;
  }

/*
** ++
**
**  Traverse the pending completion queue. For each event on the queue, check
**  if a participant (specified by its ID number) is participating.
**  Return the first event which satisfies this criteria. If an event cannot
**  be found generate a new event for construction.
**
** --
*/

EbEventBase* EbBase::_event(EbServer* server)
  {
  EbBitMask serverId;
  serverId.setBit(server->id());
  EbEventBase* event = _pending.forward();
  while( event != _pending.empty() ) {

    if(!(event->segments() & serverId).isZero() ||
       event->allocated().insert(serverId).isZero()) 
      return event;

    event = event->forward();
  }

  return _new_event(serverId);
  }

/*
** ++
**
**  Returns the event that matches the servers contribution
**
** --
*/

EbEventBase* EbBase::_seek(EbServer* srv)
{
  EbEventBase* event = _pending.forward();
  while( event != _pending.empty() ) {
    if( srv->coincides(event->key()) ) break;
    event = event->forward();
  }

  return event;
}

/*
** ++
**
**   This class is used by Eb's destructor (see below) to iterate over its
**   set of managed servers and for each server found, remove it from the
**   Event Builder's managed set and then to delete it.
**
** --
*/
#include "pds/service/ServerScan.hh"

namespace Pds {
class serverRundown : public ServerScan<Server>
  {
  public:
    serverRundown(EbBase* eb) : ServerScan<Server>(eb), _eb(eb) {}
   ~serverRundown() {}
  public:
    void process(Server* server)
    {
      _eb->_remove((EbServer*)server);
    }
  private:
    EbBase* _eb;
  };
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

EbBase::~EbBase()
  {
  serverRundown servers(this);
  servers.iterate();

  delete _ack;
  }


/*
** ++
**
**   This function is used as a diagnostic tool to perform debugging
**   in-situ. It will send to standard output a description of the
**   event builder, a description of its current set of servers, and
**   a dump of the pending queue. Note, that calling this function
**   is relativally expensive and thus should only be called sparely
**   if performance is a concern.
**
** --
*/

class serverDump : public ServerScan<Server>
  {
  public:
    serverDump(ServerManager* eb) : ServerScan<Server>(eb) {}
   ~serverDump() {}
  public:
    void process(Server* server)
    {
      EbServer* srv = (EbServer*)server;
      srv->dump(1);
    }
  };

void EbBase::_iterate_dump()
{
  serverDump servers(this);
  servers.iterate();
}

/*
** ++
**
** --
*/

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

void EbBase::dump(int detail)
  {
  time_t timeNow = time(NULL);
  printf("Dump of Event Builder %04X/%04X (log/phy)...\n",
         _id.log(), _id.phy());
  printf("--------------------------------------------\n");

  if (detail) {
    _iterate_dump();

    EbEventBase* event = _pending.forward();

    if(event != _pending.empty())
      {
      printf("Pending queue contains...\n");
      int number = 1;
      do
        {
        event->dump(number++);
        event = event->forward();
        }
      while(event != _pending.empty());
      }
    else
      printf("Pending queue is empty...\n");
    }

  printf(" Time of dump: ");
  printf(ctime(&timeNow));
  printf(" Event Timeout is %u [ms]\n", timeout());
  printf(" Has posted %u datagrams\n", _output.datagrams());
  printf(" %u Contributions, %u Chunks, %u Cache misses, %u Discards\n",
         _hits, _segments, _misses, _discards);
  _dump(detail);
  }


bool EbBase::_is_complete( EbEventBase* event,
			   const EbBitMask& serverId )
{
  EbBitMask remaining = event->remaining(serverId);
  return remaining.isZero();
}
