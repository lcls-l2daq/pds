/*
** ++
**  Package:
**      odfUtility
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
**	None.
**
** --
*/

#ifndef PDS_OUTLETWIREHEADER
#define PDS_OUTLETWIREHEADER

#include "pds/xtc/Datagram.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {
class OutletWireHeader : public Sequence
  {
  public:
    OutletWireHeader();
    OutletWireHeader(const Datagram *datagram);
  public:
    void* operator new(size_t, Pool*);
    void  operator delete(void* buffer);
  public:
    unsigned env;
    unsigned damage;
    Src      src;
    int      offset;
    int      length;
  };
}
/*
** ++
**
**
** --
*/

inline
Pds::OutletWireHeader::OutletWireHeader(const Pds::Datagram* datagram) :
  Pds::Sequence(datagram->seq, Pds::Sequence::Extended),
  env(datagram->env.value()),
  damage(datagram->xtc.damage.value()),
  src(datagram->xtc.src),
  offset(0),
  length(datagram->xtc.sizeofPayload()+sizeof(Xtc))
  {
  }

/*
** ++
**
**
** --
*/

inline void* Pds::OutletWireHeader::operator new(size_t size, Pds::Pool* pool)
  {
  return pool->alloc(sizeof(Pds::OutletWireHeader));
  }

inline void Pds::OutletWireHeader::operator delete(void* buffer)
  {
  Pds::Pool::free(buffer);
  }

#endif
