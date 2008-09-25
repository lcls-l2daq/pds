#include "ZcpEbEvent.hh"
#include "EbServer.hh"
#include "pds/xtc/ZcpDatagram.hh"

using namespace Pds;

/*
** ++
**
**   The creation of an event is initiated by the event builder, whose
**   identity is passed and saved by the event. The builder provides
**   the set of participants for this event. In addition, the builder
**   provides a buffer to contain the datagram representing the event.
**   This datagram is initialized to "empty" to represent an event
**   which has been allocated but not yet constructed. It will be
**   constructed when the first contribution sent by one of its
**   participants is received (see "consume" below). An event which
**   is pending construction is initially located on the "pending
**   construction" queue.
**   In addition, to handle the management of fragments the event contains
**   various structures including: a buffer area from which to allocate
**   segment descriptors ("_pool" and "_segments") and the listhead
**   ("_pending") of segment descriptors outstanding.
**
** --
*/

ZcpEbEvent::ZcpEbEvent(EbBitMask    creator,
		       EbBitMask    contract,
		       ZcpDatagram* zdatagram,
		       EbEventKey*  key) :
  EbEventBase   ( creator, contract, const_cast<Datagram*>(&zdatagram->datagram()), key ),
  _zdatagram    (zdatagram),
  _pending      (),
  _segments     (&_pool[0])
  {
  }

/*
** ++
**
**   This constructor is special in the sense that it does not actually
**   allocate and initialize an event. It instead, serves to initialize
**   the listhead which identifies the list of pending events. By having
**   the listhead as an event itself, various functions which must traverse
**   this list, can be written in a much more generic fashion. Note, that
**   the allocated list is set completely FULL, to spoof the "nextEvent"
**   function from attempting to allocate the listhead.
**
** --
*/

ZcpEbEvent::ZcpEbEvent() :
  EbEventBase   (),
  _zdatagram    (),
  _pending      (),
  _segments     (&_pool[0])
  {
  }


EbBitMask ZcpEbEvent::add(EbServer*        server,
			  const EbBitMask& serverId,
			  ZcpFragment*     zfrag)
{
  if ((remaining() & serverId).isZero()) { // contribution already received
    printf("ZcpEbEvent::add already has contribution %x\n",serverId.value(0));
    delete zfrag;
  }
  else if (server->more()) {  // partial datagram
    if (!(segments() & serverId).isZero()) { // already has a piece
      ZcpEbSegment* segment = _pending.forward();
      while( segment->client() != serverId )
	segment = segment->forward();

      segment = segment->consume(zfrag, 
				 server->offset());
      if (segment) { // complete now
	segment->disconnect();
	_zdatagram->fragments().insertList( &segment->fragments() );
	remaining() &= ~serverId;
	delete segment;
      }
    }
    else {
      segments() |= serverId;
      //      int length  = server->length() + (sizeof(InXtc) - sizeof(XTC));

      new (&_segments) ZcpEbSegment(server->xtc(),
				    server->offset(),
				    server->length(),
				    serverId,
				    zfrag,
				    (ZcpEbSegment*)&_pending);
    }
  }
  else { // complete contribution
    _zdatagram->fragments().insert(zfrag);
    remaining() &= ~serverId;
  }

  return remaining();
}


/*
**  Merge contributions from the two events under construction.
**  Return the mask of contributions still needed.
*/
EbBitMask ZcpEbEvent::merge(ZcpEbEvent* zev)
{
  _zdatagram->fragments().insertList( &zev->_zdatagram->fragments() );

  //  We ignore the possiblity that both events were building the same
  //  incomplete contribution.  It can't happen in the event builder.
  {
    ZcpEbSegment* empty = zev->_pending.empty();
    ZcpEbSegment* zsegm;
    while( (zsegm = zev->_pending.forward()) != empty ) {
      ZcpEbSegment* seg = new (&_segments) ZcpEbSegment(*zsegm);
      _pending.insert(seg);
    }
  }
  segments () |= zev->segments();
  remaining() &= zev->remaining();
  delete zev;
  return remaining();
}

InDatagram* ZcpEbEvent::finalize()
{
  return _zdatagram;
}

/*
** ++
**
**   This function is called (typically) by the time-out code in the
**   event builder (see "Eb.cc") to fix-up an event which has timed
**   out. The argument passed to the function is the damage value to assign
**   to the event. An event may time-out for two reasons: First, a contribution
**   never arrives. Second, one or more fragments within the contribution
**   never arrive. To handle the second case, each event keeps a list of
**   pending contributions (called segments) which are split into fragments.
**   This function will traverse this list looking for segments. For each
**   segment found, its header ("InXtc") is marked to indicate the timeout
**   and the total event size is adjusted to allow for the truncated segment.
**   Finally, the datagram describing the event is itself marked with damage
**   to indicate a time-out has taken place. The function than returns the
**   event's datagram.
**
** --
*/

void ZcpEbEvent::fixup(const Src& client, const TC& dummy)
{
  ZcpEbSegment* empty = _pending.empty();
  ZcpEbSegment* segm  = _pending.forward();
  while( segm != empty ) {
    if (segm->fixup(client,dummy))
      return;
    segm = segm->forward();
  }

  Damage damaged(1 << Damage::DroppedContribution);
  datagram()->xtc.tag.extend(sizeof(InXtc));
  InXtc xtc(dummy, client, damaged);
  _zdatagram->fragments().reverse()->uinsert(&xtc,sizeof(InXtc));
}

