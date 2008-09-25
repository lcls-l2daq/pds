/*
** ++
**  Package:
**      odfUtility
**
**  Abstract:
**      Class to define the structure of the unit of information transmitted
**      on a stream. That is, appliances either transmit or receive
**      information in units of datagrams. As an event is transmitted along
**      a stream an event is a datagram. A datagram is a container which
**      encapsulates source specific data. This source specific data is called
**      the datagram's payload. The size and location of the datagram's
**      payload is retrieved through its "sizeofPayload" and "payload"
**      functions. A datagram inherits from "odfSequence". The sequence
**      "labels" the datagram in such a fashion that makes any instantiated
**      datagram unique. Consequently, a sequence is typically a temporal
**      identifier. The datagram contains an environment ("env") whose
**      function is somehow or another describe something of the global or
**      external context in which the datagram is or was created. Its value
**      is application specific, but in a common usage is determined by
**      member functions of "odfSequencer/odfManager". The sequence "tags"
**      the datagram temporally, the "InXtc" (see member "xtc") tags the
**      datagram spatially, giving information as to its origin ("odfSrc"),
**      and the type ("odfXTC") and quality ("odfDamage") of its payload.
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

#ifndef PDS_DATAGRAM
#define PDS_DATAGRAM

#include <stdio.h>

#include "Sequence.hh"
#include "Env.hh"
#include "pds/xtc/xtc.hh"
#include "pds/service/Pool.hh"
#include "pds/service/RingPool.hh"
#include "pds/service/RingPoolW.hh"
#include "pds/collection/Transition.hh"
//#include "TcByType.hh"

namespace Pds {
class Datagram : public Sequence
  {
  public:
    Env   env;
    InXtc xtc;
  public:
    void* operator new(size_t, RingPool*,  size_t supplement = 0);
    void* operator new(size_t, RingPoolW*, size_t supplement = 0);
    void* operator new(size_t, Pool*);
    void* operator new(size_t, void*);
    void  operator delete(void* buffer);
  public:
    Datagram() {}
    Datagram(const Datagram&);
    Datagram(const Datagram&, unsigned damage);
    Datagram(const Datagram&, Sequence::ShrinkFlag);
    Datagram(const Datagram&, const TC&);
    Datagram(const Datagram&, const TC&, unsigned damage);
    //    Datagram(const TcByType&, const Src&);
    Datagram(const Datagram&, const TC&, const Src&);
    //    Datagram(const TcByType&, const Src&, unsigned damage);
    Datagram(const Sequence&, const TC&);
    Datagram(const Sequence&, const TC&, unsigned env);
    Datagram(const Sequence&, const TC&, unsigned env, const Src&);
    Datagram(const Sequence&, unsigned env);
    Datagram(const TC&);
    Datagram(const TC&, const Src&);
    Datagram(const TC&, const Src&, unsigned damage);
    Datagram(const Transition&, const TC&, const Src&);
  public:
    char*       payload()       const;
    //    int         sizeofPayload() const;
    char*       next();
    const char* next()          const;
  };
}

/*
** ++
**
**   In the interests of performance all datagrams must be instantiated
**   through purpose built allocator.  The abstract interface for this
**   allocator is defined by the class "HeapList". To enforce this
**   behaviour the datagram's "new" and "destroy" operators are overloaded
**   in such a fashion as allocate/deallocate their heap storage through
**   an "HeapList".  This particular allocator bugchecks when it runs
**   out of resources.
**
** --
*/

inline void* Pds::Datagram::operator new(size_t          size,
					 Pds::RingPool*  pool,
					 size_t          supplement)
  {
  return pool->alloc(size + supplement);
  }

inline void* Pds::Datagram::operator new(size_t          size,
					 Pds::RingPoolW* pool,
					 size_t          supplement)
  {
  return pool->alloc(size + supplement);
  }

/*
** ++
**
**   In the interests of performance all datagrams must be instantiated
**   through purpose built allocator.The abstract interface for these
**   allocator are defined by the class "Pool". To enforce this
**   behaviour the datagram's "new" and "destroy" operators are overloaded
**   in such a fashion as allocate/deallocate their heap storage through
**   an "Pool"
**
** --
*/

inline void* Pds::Datagram::operator new(size_t size, Pds::Pool* pool)
  {
  return pool->alloc(size);
  }

/*
** ++
**
**    This operator new allows one to place a datagram at a given location.
**
** --
*/

inline void* Pds::Datagram::operator new(size_t size, void* location)
  {
  return location;
  }

/*
** ++
**
**   In the interests of performance all datagrams must be instantiated
**   through purpose built allocator.The abstract interface for these
**   allocator are defined by the class "Pool". To enforce this
**   behaviour the datagram's "new" and "destroy" operators are overloaded
**   in such a fashion as allocate/deallocate their heap storage through
**   an "Pool"
**
** --
*/

inline void Pds::Datagram::operator delete(void* buffer)
  {
  Pds::RingPool::free(buffer);
  }

/*
** ++
**
**    Copy constructer...
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram& datagram) :
  Pds::Sequence(datagram),
  env(datagram.env),
  xtc(datagram.xtc)
  {
  }

/*
** ++
**
**    Copy constructer that allows the damage to be reset
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram& datagram, unsigned damage) :
  Pds::Sequence(datagram),
  env(datagram.env),
  xtc(datagram.xtc.tag, datagram.xtc.src, damage)
  {
  }

/*
** ++
**
**    Almost a copy constructer...
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram&      datagram,
			       Pds::Sequence::ShrinkFlag dummy) :
  Pds::Sequence(datagram, dummy),
  env(datagram.env),
  xtc(datagram.xtc)
  {
  }

/*
** ++
**
**    This constructor will copy all the fields of the datagram as represented
**    by its first argument, EXCEPT for it's Tagged Container Label. This
**    label is provided by the constructor's second argument. The most
**    common usage of this constructor is from within an application's FSM
**    actions (see "FSM" and "Action"), where the input transition
**    description ("Transition") should be copied into the applications's
**    output datagram.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram& datagram, const Pds::TC& tc) :
  Pds::Sequence(datagram),
  env(datagram.env),
  xtc(tc, datagram.xtc.src, datagram.xtc.damage)
  {
  }

/*
** ++
**
**    This constructor will copy all the fields of the datagram as represented
**    by its first argument, EXCEPT for it's Tagged Container Label, and the
**    damage. These are provided by the constructor's second and third arguments.
**    The most common usage of this constructor is from within an application's FSM
**    actions (see "FSM" and "Action"), where the input transition
**    description ("Transition") should be copied into the applications's
**    output datagram, but some damage has been "repaired" (i.e. masked out).
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram& datagram, 
			       const Pds::TC&   tc,
			       unsigned             damage) :
  Pds::Sequence(datagram),
  env(datagram.env),
  xtc(tc, datagram.xtc.src, damage)
  {
  }

/*
** ++
**
**    This constructor will copy all the fields of the datagram as represented
**    by its first argument, EXCEPT for its Tagged Container Label. This
**    label is provided by the constructor's second argument. The most
**    common usage of this constructor is from within an application's FSM
**    actions (see "FSM" and "Action"), where the input transition
**    description ("Transition") should be copied into the applications's
**    output datagram.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Datagram& datagram,
			       const Pds::TC&   tc,
			       const Pds::Src&      src) :
  Pds::Sequence(datagram),
  env(datagram.env),
  xtc(tc, src, datagram.xtc.damage)
  {
  }

/*
** ++
**
**    This constructor will create an empty datagram, whose Tagged Container
**    Label is set to the argument passed to the constructor.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::TC& tc) :
  Pds::Sequence(),
  env(),
  xtc(tc)
  {
  }

/*
** ++
**
**    This constructor will create an empty datagram, whose Tagged Container
**    Label and source identifier is set to the arguments passed to the
**    constructor.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::TC& tc, const Pds::Src& src) :
  Pds::Sequence(),
  env(),
  xtc(tc, src)
  {
  }

/*
** ++
**
**    This constructor will create an empty datagram, whose Tagged Container
**    Label and source identifier is set to the arguments passed to the
**    constructor.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::TC&  tc, 
			       const Pds::Src& src,
			       unsigned        damage) :
  Pds::Sequence(),
  env(),
  xtc(tc, src, damage)
  {
  }

/*
** ++
**
**    This constructor will create an empty datagram, whose sequence number
**    and Tagged Container Label is set to the arguments passed to the
**    constructor.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Sequence& sequence,
			       const Pds::TC&       tc) :
  Pds::Sequence(sequence),
  env(),
  xtc(tc)
  {
  }

/*
** ++
**
**   This constructor is used internally by DataFlow (see the inlet appliance
**   on the ROMs) and its usage is discouraged by applications. Its function
**   is to set the TC fields of the datagram predicated by its own type. The
**   type/value pairs are determined by "TcByType". In addition, it will
**   set the source identifier to a opaque value determined by its second
**   argument. Note, that this constructor still creates an empty datagram.
**
** --
*/

//inline Datagram::Datagram(const TcByType& choices,const Src& src) :
//  xtc(*choices.tc(type()), src)
//  {
//  }

/*
** ++
**
**    This constructor will create an empty datagram, whose sequence number,
**    Tagged Container Label and environment value are set to the arguments
**    passed to the constructor.
**
** --
*/

inline Pds::Datagram::Datagram(const Pds::Sequence& sequence,
			       const Pds::TC&   tc,
			       unsigned             _env) :
  Pds::Sequence(sequence),
  env(_env),
  xtc(tc)
  {
  }

inline Pds::Datagram::Datagram(const Pds::Sequence& sequence,
			       const Pds::TC&   tc,
			       unsigned             _env,
			       const Pds::Src&      src) :
  Pds::Sequence(sequence),
  env(_env),
  xtc(tc,src,Pds::Damage(0))
  {
  }

/*
** ++
**
**    This constructor will create an empty datagram, whose sequence number
**    and environment value are set to the arguments passed to the constructor.
**    The Tagged Container label describing the payload is set to "unknown".
**
** --
*/


inline Pds::Datagram::Datagram(const Pds::Sequence& sequence, unsigned _env) :
  Pds::Sequence(sequence),
  env(_env),
  xtc()
  {
  }

inline Pds::Datagram::Datagram(const Pds::Transition& tr,
                               const TC& tc,
                               const Src& src) :
  Pds::Sequence(tr.sequence()),
  env          (tr.env()),
  xtc          (tc,src)
{
}

/*
** ++
**
**    Returns an unspecified pointer to the datagram's payload.
**
** --
*/

inline char* Pds::Datagram::payload() const
  {
  return (char*)&this[1];
  }

/*
** ++
**
**    Returns the size (in bytes) of the datagram's payload
**
** --
*/

//inline int Pds::Datagram::sizeofPayload() const
//  {
//  return xtc.tag.extent();
//  }

/*
** ++
**
**    Returns the location of the datagram which would immediately follow
**    this datagram (if the datagrams were contiguous). This function is
**    useful if the application wishes to construct an iterator over a
**    collection of such contiguous datagrams.
**
** --
*/

inline char* Pds::Datagram::next()
  {
  return (char*)xtc.next();
  }

/*
** ++
**
**    const version of the above.
**
** --
*/

inline const char* Pds::Datagram::next() const
  {
  return (const char*)xtc.next();
  }

#endif
