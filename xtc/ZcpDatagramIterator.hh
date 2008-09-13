#ifndef Pds_ZcpDatagramIterator_hh
#define Pds_ZcpDatagramIterator_hh

#include "InDatagramIterator.hh"
#include "pds/service/LinkedList.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/service/RingPool.hh"

namespace Pds {

  class Datagram;
  class ZcpDatagram;

  class ZcpDatagramIterator : public InDatagramIterator {
  public:
    ZcpDatagramIterator( const ZcpDatagram&, Pool* );
    ~ZcpDatagramIterator();

    void* operator new(unsigned,Pool*);
    void  operator delete(void*);

    int skip(int len);
    int read(iovec* iov, int maxiov, int len);
    int copy(void*, int len);
  private:
    void _unmap_iovs();

    const Datagram&         _datagram;
    LinkedList<ZcpFragment> _fragments;

    class IovBuffer {
    public:
      IovBuffer();
      ~IovBuffer();
      
      void     unmap ();
      int      bytes () const { return _bytes; }
      int      remove(int bytes);
      int      remove(iovec*,int maxiov,int bytes);
      int      insert(ZcpFragment&, int bytes);
    private:
      enum { MAXIOVS = 64 }; // maximum iovs allowed in one ZcpFragment (pipe)
      unsigned _bytes;   // number of bytes mapped but not returned to user
      unsigned _iiov;    // highest _iov returned to user
      unsigned _iiovlen; // length of _iiov segment not returned to user
      unsigned _niov;    // next _iov to fill
      iovec    _iovs[MAXIOVS];
    } _iovbuff;
  };
}

inline void* Pds::ZcpDatagramIterator::operator new(unsigned sz,Pool* pool)
{
  return pool->alloc(sz);
}

inline void Pds::ZcpDatagramIterator::operator delete(void* buffer)
{
  return Pds::RingPool::free(buffer);
}

#endif
