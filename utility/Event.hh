/*
** ++
**  Package:
**	odfClient
**
**  Abstract:
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**	gpdf 98.03.11 - Modify odfTransition::args() to use new OdfContainer
**                      convention for extent().
**
** --
*/

#ifndef PDS_EVENT
#define PDS_EVENT
  
#include "pds/xtc/Datagram.hh"
#include "EventId.hh"
//#include "TypeIdPackage.hh"
#include "pds/xtc/xtc.hh"

namespace Pds {
  class Event : public Datagram
  {
  public:
    Event(Pds::EventId::Value, unsigned env);

    unsigned              args () const;
    EventId::Value        event() const;
    static EventId::Value event( const Sequence& s);
  };
}

/*
** ++
**
**
** --
*/

inline
Pds::Event::Event(Pds::EventId::Value name, unsigned env1) :
  Datagram(
	Pds::Sequence(Pds::Sequence::Event,
		      (Pds::Sequence::Service)(name),
		      (unsigned)0,
		      (unsigned)0),
	env1)
  {
  }


/*
** ++
**
**
** --
*/

inline Pds::EventId::Value Pds::Event::event()  const
  {
  return (Pds::EventId::Value)(service());
  }

/*
** ++
**
** --
*/

inline unsigned Pds::Event::args() const
  {
  // Need to compensate for the seven (presently) longwords occupied by
  // the Transition object itself (new semantics of extent).  This is
  // a place where there's an additional coding cost.
  return xtc.sizeofPayload() >> 2;
  }

/*
** ++
**
** --
*/

inline Pds::EventId::Value Pds::Event::event(const Pds::Sequence& s)
  {
  return (s.type() == Pds::Sequence::Event                                  ?
          (Pds::EventId::Value)(s.service()) :
          Pds::EventId::Unknown );
  }

#endif
