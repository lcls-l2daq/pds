#include "EbEvent.hh"
#include "EbSegment.hh"
#include "EbServer.hh"
#include "pds/xtc/CDatagram.hh"

#include <stdio.h>

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

EbEvent::EbEvent(EbBitMask creator,
		 EbBitMask contract,
		 CDatagram* datagram,
		 EbEventKey* key) :
  EbEventBase   (creator, contract, const_cast<Datagram*>(&datagram->datagram()), key),
  _cdatagram    (datagram),
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

EbEvent::EbEvent() :
  EbEventBase   (),
  _pending      (),
  _segments     (&_pool[0])
  {
  }

/*
** ++
**
**   This function will determine whether an event has a contribution pending
**   due to the (imminent) arrival of a segment. A segment is one fraction
**   of a datagram which because of transport limitation could not be sent
**   in a single contiguous piece. Each event keeps a queue (by client) of
**   pending segments. This function will search the queue for a segment
**   which matches the client specified as an argument to this function.
**   If a match is found a pointer to the specified segment is returned.
**   If not, or the pending queue is empty a NIL (zero) pointer is returned.
**
** --
*/

EbSegment* EbEvent::hasSegment(EbBitMask id)
{
  if ((segments() & id).isZero())  return (EbSegment*)0;

  EbSegment* segment = _pending.forward();

  while(segment != _pending.empty())
    {
    if(id == segment->client()) return segment;
    segment = segment->forward();
    }
  return (EbSegment*)0;
}

/*
** ++
**
**   This function is called for each arrived datagram (see "EbServer").
**   Its function is to update the output datagram which represents the
**   data being built. The function is passed both a pointer to the in-coming
**   datagram ("in") and the size and location of its payload ("sizeofPayload")
**   and "payload"). These values are used to update the output datagram's
**   damage and size. In addition, the arrived datagram may represent not the
**   whole contribution, but rather just a fragment of it. If this is the case
**   the in-coming datagram has its incoherent bit SET (I've hijacked this
**   bit for this purpose, it should be renamed, BEWARE!). If the in-coming
**   datagram is a fragment its header is actually described by a different
**   structure. This structure differs from a datagram only in that its
**   TC fields contain the offset (relative to the contribution) that the
**   fragment is expected to be located. If the datagram is a fragment,
**   this function must treat two cases: First, a fragment for this event,
**   from this contributor has not yet seen.If this is the case, a segment
**   descriptor is allocated and constructed to manage the assembly of
**   fragments into a single contribution. Otherwise, the fragment is passed
**   to an existing segment descriptor which updates the segment with
**   the information from the just arrived fragment. These two cases
**   are differentiated by passing in a pointer to a segment descriptor
**   ("inProgress") which is NIL (zero) if assembly is not in-progress.
**   The function returns a boolean indicating if more fragments are expected.
**
** --
*/

bool EbEvent::consume(const EbServer* srv,
		      int sizeofPayload,
		      EbBitMask client)
{
  Datagram* out = datagram();

  const Xtc& xtc = srv->xtc();

  out->xtc.damage.increase(xtc.damage.value() & ~(1<<Damage::IncompleteContribution));

  if(srv->more()) {
    unsigned   offset  = srv->offset();
    EbSegment* segment = hasSegment(client);
    if(segment) {
      segment->consume(sizeofPayload, offset, xtc);
      switch(segment->complete()) {
      case EbSegment::IsComplete:
	segments() &= ~client;
	return false;
      case EbSegment::WontComplete:
	out->xtc.damage.increase(1<<Damage::ContainsIncomplete);
	return false;
      case EbSegment::MayComplete:
	return true;
      }
    }

    segments() |= client;
    int length = srv->length();
    int size   = sizeofPayload;

    char* location = (char*)out->xtc.alloc(length);
    segment = new(&_segments) EbSegment(xtc,
					location,
					size,
					offset,
					length,
					client,
					(EbSegment*)&_pending);
    switch(segment->complete()) {
    case EbSegment::IsComplete:
      segments() &= ~client;
      return false;
    case EbSegment::WontComplete:
      out->xtc.damage.increase(1<<Damage::ContainsIncomplete);
      return false;
    case EbSegment::MayComplete:
      return true;
    }
  }
  
  out->xtc.alloc(sizeofPayload);
  return false;
}

//
//  The payload from client was mistakenly received in this event.  Remove any
//  new allocation marking and check for potential overwrites.  Mark overwritten
//  event with IncompleteContribution damage (unsafe to iterate upon).
//
EbBitMask EbEvent::deallocate(EbBitMask client, char* payload, int sizeofPayload)
{
  if ((segments() & client).isZero())       // Data received at the end of the event;
    return EbEventBase::deallocate(client); // nothing previously from this client.

  EbSegment* segment = hasSegment(client);  // Data received at the end of an existing contribution.
  if (segment) {                            // Check if contribution container bounds exceeded.
    if (!segment->deallocate(payload,sizeofPayload))
      datagram()->xtc.damage.increase(Damage::IncompleteContribution);
  }
  else                                      // Data received at the end of a complete contribution.
    datagram()->xtc.damage.increase(Damage::IncompleteContribution);

  return allocated().remaining();  // Do not remove allocation mark (previous contribution exists).
}

/*
** ++
**
**    The TCP/IP socket interface requires a buffer whenever a read is
**    hung. As a consequence, memory for event contributions must be
**    allocated one contribution ahead, even though in actuality it
**    cannot be known apriori WHICH event the contribution will be
**    destined for. This function is called whenever the educated guess
**    for the read goes bad. Its function is to copy the contribution's
**    payload from the wrong event to the right event (i.e., this object).
**    Note, that the function assumes that even though it is passed the
**    payload size in bytes, that the size is actually an integral number
**    of longwords (4 byte quantities)... The function returns a pointer
**    to the buffer that now contains the payload.
**
** --
*/

char* EbEvent::recopy(char* payloadIn, int sizeofPayload, EbBitMask client)
  {
  unsigned remaining  = (unsigned)sizeofPayload;
  char*    outPayload = payload(client);

  remaining >>= 2;

  if(remaining)
    {
    unsigned* in  = (unsigned*)payloadIn;
    unsigned* out = (unsigned*)outPayload;
    do *out++ = *in++; while(--remaining);
    }

  return outPayload;
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

unsigned EbEvent::fixup(const Src& client, const EbBitMask& id)
  {
  EbSegment* segment = hasSegment(id);
  if(segment)
    {
    return segment->fixup();
    }
  else
    {
    Damage damaged(1 << Damage::DroppedContribution);
    // kludge away the bld dummy contribution until we understand
    // data corruption. - weaver,cpo
    //    if (client.level()!=Level::Reporter) 
      new(&datagram()->xtc) Xtc(TypeId(TypeId::Any,0), client, damaged);
    return damaged.value();
    }
  }

InDatagram* EbEvent::finalize()
{
  return cdatagram();
}

#include <stdio.h>

void EbEvent::dump() const
{
  EbEvent* event = const_cast<EbEvent*>(this);
  printf("  alloc %08x rem %08x\n",
         event->allocated().remaining().value(0),
         event->remaining().value(0));
  printf("  segm %08x\n", 
         event->segments().value(0));

  for(EbSegment* segment = _pending.forward();
      segment != _pending.empty(); 
      segment = segment->forward()) 
    printf(" [%08x]",segment->client().value(0));
  if (_pending.forward()!=_pending.empty())
    printf("\n");

  printf("  dgm seq %08x ext %08x\n",
         event->datagram()->seq.stamp().fiducials(),
         event->datagram()->xtc.extent);
}
