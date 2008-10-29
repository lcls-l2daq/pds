/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**	Class declaration for odfOccurrence. An occurence is an object
**      which describes an abnormal or execptional condtion occuring
**      somewhere within the DataFlow hierarchy. Occurences are actually
**      just datagrams carried on a stream.
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_OCCURRENCE
#define PDS_OCCURRENCE

#include "pds/xtc/Datagram.hh"
#include "OccurrenceId.hh"
#include "Time.hh"

namespace Pds {
class Occurrence : public Datagram
  {
  public:
    friend class OccurrenceStack;
    Occurrence(OccurrenceId::Value, unsigned reason);
    Occurrence(OccurrenceId::Value, unsigned reason,
	       const Time& time);
    Occurrence(OccurrenceId::Value, unsigned reason,
		  const Sequence& sequence);
    Occurrence(OccurrenceId::Value, unsigned reason,
		  const Sequence& sequence, const Src& src);
    static Occurrence* create(OccurrenceId::Value,
                                 unsigned reason,
                                 Pool* pool);
    OccurrenceId::Value type()   const;
    unsigned               reason() const;
    unsigned               arguments() const;
    int                    push(unsigned);
  private:
    enum {maxArgs = 4};
    unsigned _stack[maxArgs];
  };
}
/*
** ++
**
** --
*/

inline
Pds::Occurrence::Occurrence(Pds::OccurrenceId::Value name, unsigned reason) :
  Pds::Datagram(Pds::Sequence(Pds::Sequence::Occurrence,
			      (Pds::Sequence::Service)name,
			      (unsigned)0xffffffff,
			      (unsigned)0x00ffffff),
		Pds::XTC(),
		reason)
  {
  }

/*
** ++
**
** --
*/


inline
Pds::Occurrence::Occurrence(Pds::OccurrenceId::Value name, unsigned reason,
			    const Pds::Time& time) :
  Pds::Datagram(Pds::Sequence(Pds::Sequence::Occurrence,
			      (Pds::Sequence::Service)name,
			      (unsigned)time.binary().lower,
			      (unsigned)time.binary().upper),
		Pds::XTC(),
		reason)
  {
  }

/*
** ++
**
** --
*/

inline
Pds::Occurrence::Occurrence(Pds::OccurrenceId::Value name, unsigned reason,
			    const Pds::Sequence& sequence) :
  Pds::Datagram(Pds::Sequence(Pds::Sequence::Occurrence,
			      (Pds::Sequence::Service)name,
			      (unsigned)sequence.low(),
			      (unsigned)sequence.high()),
		Pds::XTC(),
		reason)
  {
  }

inline
Pds::Occurrence::Occurrence(Pds::OccurrenceId::Value name, unsigned reason,
			    const Pds::Sequence& sequence, const Pds::Src& src) :
  Pds::Datagram(Pds::Sequence(Pds::Sequence::Occurrence,
			      (Pds::Sequence::Service)name,
			      (unsigned)sequence.low(),
			      (unsigned)sequence.high()),
		Pds::XTC(),
		reason,
		src)
  {
  }

/*
** ++
**
** --
*/

inline Pds::Occurrence* Pds::Occurrence::create(Pds::OccurrenceId::Value name,
						unsigned reason,
						Pds::Pool* pool)
  {
  return new(pool) Pds::Occurrence(name, reason, Pds::Time(0x00ffffff, 0xffffffff));
  }

/*
** ++
**
** --
*/

inline unsigned Pds::Occurrence::reason() const
  {
  return env.value();
  }

/*
** ++
**
** --
*/

inline Pds::OccurrenceId::Value Pds::Occurrence::type() const
  {
  return (Pds::OccurrenceId::Value)service();
  }

/*
** ++
**
** --
*/

inline unsigned Pds::Occurrence::arguments() const
  {
  return xtc.sizeofPayload() >> 2;
  }

/*
** ++
**
** --
*/

inline int Pds::Occurrence::push(unsigned value)
  {
  unsigned sp = arguments();
  if(sp < maxArgs)
    {
    _stack[sp] = value;
    xtc.tag.extend(sizeof(unsigned));
    return 1;
    }
  else
    return 0;
  }

/*
** ++
**
** --
*/

namespace Pds {
class OccurrenceStack
  {
  public:
    OccurrenceStack(const Occurrence*);
    OccurrenceStack(const Xtc*);
    unsigned pop();
  private:
    const unsigned* _next;
  };
}

/*
** ++
**
** --
*/

inline Pds::OccurrenceStack::OccurrenceStack(const Pds::Occurrence* occurrence) :
  _next(&occurrence->_stack[occurrence->arguments()])
  {
  }

/*
** ++
**
** --
*/

inline Pds::OccurrenceStack::OccurrenceStack(const Pds::Xtc* occurrence)
  {
    unsigned* next = (unsigned*) occurrence->payload();
    next += occurrence->sizeofPayload() >> 2; 
    _next = next;					       
  }

/*
** ++
**
** --
*/

inline unsigned Pds::OccurrenceStack::pop()
  {
  return *--_next;
  }

#endif
