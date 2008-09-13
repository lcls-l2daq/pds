/*
** ++
**  Package:
**	odfConcentrator
**
**  Abstract:
**
**      This class defines the context necessary to manage an event
**      as it it makes the phase changes between "construction",
**      "completing", and "completion".
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

#ifndef PDS_EBEVENTBASE_HH
#define PDS_EBEVENTBASE_HH

#include "pds/xtc/Sequence.hh"
#include "pds/xtc/Datagram.hh"
#include "EbClients.hh"
#include "EbSegment.hh"
#include "EbKey.hh"
#include "pds/service/LinkedList.hh"

namespace Pds {

class EbServer;
class EbTimeouts;
class InDatagram;

class EbEventBase : public LinkedList<EbEventBase>
  {
  public:
    EbEventBase();
    EbEventBase(EbBitMask creator, EbBitMask contract, Datagram*);
    virtual ~EbEventBase();
  public:
    Datagram*        datagram  ()  const;
    EbBitMask        remaining () const;
    EbBitMask        remaining (EbBitMask id);
    void             add       (const EbPulseId*, EbEventBase* after);
    void             dump      (int number);
    EbBitMask        deallocate(EbBitMask id);
    int              timeouts  (const EbTimeouts& ebtimeouts);
    EbKey&           key       ();
    const EbBitMask& segments  () const;
    EbBitMask&       segments  ();
    EbClients&       allocated ();
  public:
    virtual InDatagram* finalize() = 0;

    //    EbSegment* consume(const EbServer*,
    //		       int sizeofPayload,
    //		       EbBitMask client);
    //    EbSegment* hasSegment(EbBitMask client);
    //    char*         recopy(char* payload, int sizeofPayload, EbBitMask server);
    //    void          fixup(const Src&, const TC&);
    //    char*         payload(EbBitMask client);
  private:
    EbKey         _key;
    EbClients     _allocated;     // Clients which have allocated event
    EbClients     _contributions; // List of clients yet to contribute
    EbBitMask     _contract;      // -> potential list of contributors
    EbBitMask     _segments;      // Clients for which a segment already exists
    int           _timeouts;      // How many counts before event timeouts
    Datagram*     _datagram;
  };
}

/*
** ++
**
**    As soon as an event becomes "complete" its datagram is the only
**    information of value within the event. Therefore, when the event
**    becomes complete it is deleted which cause the destructor to
**    remove the event from the pending queue.
**
** --
*/

inline Pds::EbEventBase::~EbEventBase()
  {
  disconnect();
  }

/*
** ++
**
**    Each event has a datagram associated with it. This datagram is actually
**    the event as seen outside the event building appliance and is constitutes
**    the data structure the event builder is actually building. This function
**    returns a pointer to that datagram.
**
** --
*/

inline Pds::Datagram* Pds::EbEventBase::datagram() const
  {
  return _datagram;
  }

/*
** ++
**
**    The event maintains a bit-list of the contributions (by ID) which
**    remain to move the event from the state of "under completion", to
**    "completed". This function returns whether the event is "under
**    completion" (a positive value), or "completed". Since an event
**    under CONSTRUCTION has by definition no contributors (yet), this
**    function is commonly used by those functions which traverse the
**    event pending list to filter out "under construction" events.
**
** --
*/

inline EbBitMask Pds::EbEventBase::remaining() const
  {
  return _contributions.remaining();
  }

/*
** ++
**
**   This function is called for each arrived datagram (see "EbServer").
**   Its function is to mark off the datagram's contributor, and to determine
**   whether this event, currently under construction is "complete" (a complete
**   event has seen all its possible contributors). Note, that the "remove"
**   function returns a zero (0) value if there are no more contributions
**   remaining and a non-zero value if contributions remain. Therefore, a
**   ZERO value returned by this function signifies that the event is complete.
**
** --
*/

inline EbBitMask Pds::EbEventBase::remaining(EbBitMask id)
  {
  return _contributions.remove(id);
  }

inline const EbBitMask& Pds::EbEventBase::segments() const
{
  return _segments;
}

inline EbBitMask& Pds::EbEventBase::segments()
{
  return _segments;
}

/*
** ++
**
**    The "_allocated" object keeps track whether a particular server has
**    referenced the event. Since the server attempts to cache it's next
**    event, it may have temporally a reference to an event which it must
**    relinquish. This function exists for this purpose. Its takes as an
**    an argument the ID of the server whose reference is to be removed.
**    It returns the list of servers which still remain a reference to
**    the event.
**
** --
*/

inline EbBitMask Pds::EbEventBase::deallocate(EbBitMask id)
  {
  return _allocated.remove(id);
  }

inline Pds::EbKey& Pds::EbEventBase::key()
{
  return _key;
}

inline Pds::EbClients& Pds::EbEventBase::allocated()
{
  return _allocated;
}

#endif
