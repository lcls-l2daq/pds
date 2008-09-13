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
			 Datagram* datagram) :
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
  _allocated    (EbBitMask(EbBitMask::FULL)),
  _contributions(),
  _contract     (),
  _segments     (),
  _timeouts     (MaxTimeouts),
  _datagram     ((Datagram*)0)
  {
  }

/*
** ++
**
**   This function will insert ourselves on the Event Builder's pending
**   queue. This queue is sorted by sequence number, from old (smallest
**   value), to new (largest value). The entry AFTER which we are to be
**   inserted is specified by the "after" argument. Since we (by definition)
**   have just come into existence our (and our datagram's) sequence
**   number have not yet been established. The first argument ("key")
**   defines their value and this function will also initialize these
**   values.
**
** --
*/

void EbEventBase::add(const EbPulseId* key, EbEventBase* after)
  {
  Datagram* datagram = _datagram;
  Sequence* sequence = datagram;
  //  datagram->env = key->env;
  *sequence          = Sequence(Sequence::Event, Sequence::L1Accept,
				key->_data[0], key->_data[1]);
  _key.pulseId       = *key;
  _key.types        |= (1<<EbKey::PulseId);
  connect(after);
#ifdef VERBOSE
  printf("EbEventBase::add %p  forward %p  reverse %p\n",
	 this, this->forward(), this->reverse());
#endif
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
    int tmo = ebtmo.timeouts(_datagram);
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
  Sequence& sequence = (Sequence&)_key.pulseId;
  printf(" Event #%d @ address %08X has sequence %08X%08X service %X type %X\n",
         number,
         (int)this,
         sequence.highAll(),
         sequence.low(),
         sequence.service(),
         sequence.type());
  printf("   Forward link -> %08X, Backward link -> %08X\n",
         (unsigned)forward(), (unsigned)reverse());

  printf("   Contributions seen/requested = ");
  for (unsigned i = EbBitMask::BitMaskWords; i > 0; i--)
    printf("%08x", _contributions.remaining().value(i-1));
  printf("/");
  for (unsigned j = EbBitMask::BitMaskWords; j > 0; j--)
    printf("%08x", _contract.value(j-1));
  printf("\n");
  printf("   Allocations   seen/requested = ");
  for (unsigned k = EbBitMask::BitMaskWords; k > 0; k--)
    printf("%08x", _allocated.remaining().value(k-1));
  printf("/");
  for (unsigned l = EbBitMask::BitMaskWords; l > 0; l--)
    printf("%08x", _contract.value(l-1));
  printf("\n");

  printf("   Datagram address = %d, %08X (hex)\n",
         (int)_datagram, (int)_datagram);
  }
