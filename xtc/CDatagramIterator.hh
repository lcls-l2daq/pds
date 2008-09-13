#ifndef Pds_CDatagramIterator_hh
#define Pds_CDatagramIterator_hh

#include "InDatagramIterator.hh"
#include "pds/service/RingPool.hh"

namespace Pds {

  class Datagram;

  class CDatagramIterator : public InDatagramIterator {
  public:
    CDatagramIterator( const Datagram& dg);
    ~CDatagramIterator();

    void* operator new(unsigned,Pool*);
    void  operator delete(void*);

    int skip(int len);
    int read(iovec* iov, int maxiov, int len);
    int copy(void* buff, int len);
  private:
    const Datagram& _datagram;
    int             _offset;
    int             _end;
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
  _datagram(dg),
  _offset  (sizeof(Datagram)),
  _end     (sizeof(Datagram) + dg.xtc.sizeofPayload())
{
}

#endif
