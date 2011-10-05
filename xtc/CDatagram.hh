#ifndef Pds_CDatagram_hh
#define Pds_CDatagram_hh

#include "InDatagram.hh"
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
    //void* operator new(size_t, void*);
    void  operator delete(void* buffer);

    Datagram& dg();

    bool insert(const Xtc& tc, const void* payload);

    InDatagramIterator* iterator(Pool*) const;

    int  send(ToEb&);
    int  send(ToNetEb&, const Ins&);
    int  unblock(OobServer&, char*);

    TrafficDst* traffic(const Ins&);
  };
}


inline Pds::CDatagram::CDatagram(const Datagram& dg) :
  InDatagram(dg)
{
  xtc.damage = 0;
}

inline Pds::CDatagram::CDatagram(const Datagram& dg,
				 const Xtc& tc) :
  InDatagram(dg)
{
  int size = tc.sizeofPayload();
  memcpy(xtc.alloc(size),tc.payload(),size);
}

inline Pds::CDatagram::CDatagram(const TypeId& type, const Src& src) :
  InDatagram(Datagram(type,src))
{
}

inline Pds::CDatagram::CDatagram(const Datagram& dg, const TypeId& ctn,
                                 const Src& src) :
  InDatagram(Datagram(dg,ctn,src))
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

//inline void* Pds::CDatagram::operator new(size_t size, void* pMem)
//{
//  return pMem;
//}

inline void Pds::CDatagram::operator delete(void* buffer)
{
  Pds::RingPool::free(buffer);
}

inline Pds::Datagram& Pds::CDatagram::dg() { return *this; }


#endif
