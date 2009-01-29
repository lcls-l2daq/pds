#ifndef Pds_CDatagramIterator_hh
#define Pds_CDatagramIterator_hh

#include "PayloadIterator.hh"
#include "pds/service/RingPool.hh"

namespace Pds {

  class Datagram;

  class CDatagramIterator : public PayloadIterator {
  public:
    CDatagramIterator( const Datagram& dg);
    ~CDatagramIterator();

    void* operator new(unsigned,Pool*);
    void  operator delete(void*);
 };

}

inline void* Pds::CDatagramIterator::operator new(unsigned sz,Pool* pool)
{
  return pool->alloc(sz);
}

inline void Pds::CDatagramIterator::operator delete(void* buffer)
{
  return Pds::RingPool::free(buffer);
}

inline Pds::CDatagramIterator::CDatagramIterator(const Datagram& dg) :
  PayloadIterator( reinterpret_cast<const char*>(&dg + 1), dg.xtc.sizeofPayload() )
{
}

inline Pds::CDatagramIterator::~CDatagramIterator()
{
}

#endif
