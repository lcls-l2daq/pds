#ifndef Pds_CDatagram_hh
#define Pds_CDatagram_hh

#include "InDatagram.hh"
#include "Datagram.hh"
#include "pds/xtc/xtc.hh"
#include <string.h>

namespace Pds {

  class CDatagram : public InDatagram {
  public:
    CDatagram() {}
    CDatagram(const Datagram&);
    CDatagram(const Datagram&, const InXtc&);
    CDatagram(const TC&, const Src&);
    ~CDatagram();

    void* operator new(size_t, Pool*);
    void* operator new(size_t, RingPool*);
    void  operator delete(void* buffer);

    const Datagram& datagram() const;

    InDatagramIterator* iterator(Pool*) const;

    int  send(ToEb&);
    int  send(ToNetEb&, const Ins&);
    int  unblock(OobServer&, char*);
    
  private:
    Datagram  _datagram;
  };
}


inline Pds::CDatagram::CDatagram(const Datagram& dg) :
  _datagram(dg,dg.xtc.tag,dg.xtc.src)
{
}

inline Pds::CDatagram::CDatagram(const Datagram& dg,
				 const InXtc& xtc) :
  _datagram(dg)
{
  int size = xtc.sizeofPayload();
  memcpy(_datagram.xtc.tag.alloc(size),xtc.payload(),size);
}

inline Pds::CDatagram::CDatagram(const TC& tc, const Src& src) :
  _datagram(tc,src)
{
}

inline void* Pds::CDatagram::operator new(size_t size, Pds::RingPool* pool)
  {
  return pool->alloc(size);
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
