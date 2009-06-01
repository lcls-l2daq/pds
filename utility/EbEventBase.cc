#include "EbEventBase.hh"
#include "EbServer.hh"
#include "EbTimeouts.hh"

static const int MaxTimeouts=0xffff;

//#define VERBOSE

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

EbEventBase::EbEventBase(EbBitMask creator,
			 EbBitMask contract,
			 Datagram* datagram,
			 EbEventKey* key) :
  _key          (key),
  _allocated    (creator),
  _contributions(contract),
  _contract     (contract),
  _segments     (),
  _timeouts     (MaxTimeouts),
  _datagram     (datagram)
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

EbEventBase::EbEventBase() :
  LinkedList<EbEventBase>(), 
  _key          (0),
  _allocated    (EbBitMask(EbBitMask::FULL)),
  _contributions(),
  _contract     (),
  _segments     (),
  _timeouts     (MaxTimeouts),
  _datagram     (0)
  {
  }

/*
** ++
**
**    
**
** --
*/

int EbEventBase::timeouts(const EbTimeouts& ebtmo) {
  if (_timeouts == MaxTimeouts) {
    int tmo = ebtmo.timeouts((Sequence*)0);
    _timeouts = tmo < MaxTimeouts ? tmo : MaxTimeouts;
  }
  return --_timeouts;
}


/*
** ++
**
**    Simple debugging tool to format and dump the contents of the object...
**
** --
*/

#include <stdio.h>

void EbEventBase::dump(int number)
  {
    /*
  Sequence& sequence = (Sequence&)_key.pulseId;
  printf(" Event #%d @ address %08X has sequence %08X%08X service %X type %X\n",
         number,
         (int)this,
         sequence.highAll(),
         sequence.low(),
         sequence.service(),
         sequence.type());
    */
  printf(" Event #%d @ address %p\n",
	 number, this);
  printf("   Forward link -> %08X, Backward link -> %08X\n",
         (unsigned)forward(), (unsigned)reverse());
  printf("   Contributions seen/requested = ");
  _contributions.remaining().print();
  printf("/");
  _contract.print();
  printf("\n");
  printf("   Allocations   seen/requested = ");
  _allocated.remaining().print();
  printf("/");
  _contract.print();
  printf("\n");

  //  printf("   Datagram address = %d, %08X (hex)\n",
  //         (int)_datagram, (int)_datagram);
  }

