#ifndef Pds_CDatagramIterator_hh
#define Pds_CDatagramIterator_hh

#include "PayloadIterator.hh"
#include "pds/service/Pool.hh"
#include "pds/xtc/Datagram.hh"

namespace Pds {

  class Datagram;

  class CDatagramIterator : public PayloadIterator {
  public:
    CDatagramIterator( const Datagram& dg);
    ~CDatagramIterator();

    void* operator new(size_t,Pool*);
    void  operator delete(void*);
 };

}

inline void* Pds::CDatagramIterator::operator new(size_t sz,Pool* pool)
{
  return pool->alloc(sz);
}

inline void Pds::CDatagramIterator::operator delete(void* buffer)
{
  return Pds::Pool::free(buffer);
}

inline Pds::CDatagramIterator::CDatagramIterator(const Datagram& dg) :
  PayloadIterator( reinterpret_cast<const char*>(&dg + 1), dg.xtc.sizeofPayload() )
{
}

inline Pds::CDatagramIterator::~CDatagramIterator()
{
}

#endif
