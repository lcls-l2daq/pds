#ifndef Pds_CDatagram_hh
#define Pds_CDatagram_hh

#include "InDatagram.hh"
#include "Datagram.hh"
#include "pds/service/Pool.hh"
#include "pds/service/RingPool.hh"

#include <string.h>

namespace Pds {

  class CDatagram : public InDatagram {
  public:
    CDatagram(const Datagram&);
    CDatagram(const Datagram&, const Xtc&);
    CDatagram(const TypeId&, const Src&);
    CDatagram(const Datagram& dg, const TypeId& ctn, const Src& src);
    ~CDatagram();

    void* operator new(size_t, Pool*);
    void* operator new(size_t, RingPool*);
    void  operator delete(void* buffer);

    const Datagram& datagram() const;
    Datagram& datagram();
    Datagram& dg();

    bool insert(const Xtc& tc, const void* payload);

    InDatagramIterator* iterator(Pool*) const;

    int  send(ToEb&);
    int  send(ToNetEb&, const Ins&);
    int  unblock(OobServer&, char*);
    
  private:
    Datagram  _datagram;
  };
}


inline Pds::CDatagram::CDatagram(const Datagram& dg) :
  _datagram(dg,dg.xtc.contains,dg.xtc.src)
{
}

inline Pds::CDatagram::CDatagram(const Datagram& dg,
				 const Xtc& xtc) :
  _datagram(dg)
{
  int size = xtc.sizeofPayload();
  memcpy(_datagram.xtc.alloc(size),xtc.payload(),size);
}

inline Pds::CDatagram::CDatagram(const TypeId& type, const Src& src) :
  _datagram(type,src)
{
}

inline Pds::CDatagram::CDatagram(const Datagram& dg, const TypeId& ctn,
                                 const Src& src) :
  _datagram(dg,ctn,src)
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
