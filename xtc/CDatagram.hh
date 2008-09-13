#ifndef Pds_CDatagram_hh
#define Pds_CDatagram_hh

#include "InDatagram.hh"
#include "Datagram.hh"
#include "pds/xtc/xtc.hh"
#include <string.h>

namespace Pds {

  class CDatagram : public InDatagram {
  public:
    CDatagram(const Datagram&);
    CDatagram(const TC&, const Src&);
    ~CDatagram();

    void* operator new(size_t, Pool*);
    void  operator delete(void* buffer);

    const Datagram& datagram() const;

    InDatagramIterator* iterator(Pool*) const;

    int send(ToEb&, const Ins&);

  private:
    Datagram  _datagram;
  };
}


inline Pds::CDatagram::CDatagram(const Datagram& dg) :
  _datagram(dg)
{
  int size = dg.xtc.sizeofPayload();
  memcpy(_datagram.xtc.tag.alloc(size),dg.payload(),size);
}

inline Pds::CDatagram::CDatagram(const TC& tc, const Src& src) :
  _datagram(tc,src)
{
}

inline void* Pds::CDatagram::operator new(size_t size, Pds::Pool* pool)
  {
  return pool->alloc(size);
  }

inline void Pds::CDatagram::operator delete(void* buffer)
  {
  Pds::RingPool::free(buffer);
  }


#endif
