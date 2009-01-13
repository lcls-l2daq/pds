#ifndef Pds_ZcpDatagramIterator_hh
#define Pds_ZcpDatagramIterator_hh

//
//  ZcpDatagramIterator
//
//    Provides user-space access to the datagram.  The datagram
//    memory is mapped to user-space in response to "read" requests.
//    The lifetime of the mapping is equivalent to the lifetime of
//    this iterator.  A performance improvement might be to tie the
//    lifetime of the mapping to the lifetime of the datagram itself,
//    assuming the mapping is any faster a second time.
//
#include "InDatagramIterator.hh"
#include "pds/service/LinkedList.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/service/KStream.hh"
#include "pds/service/RingPool.hh"
#include "pds/xtc/ZcpDatagram.hh"

namespace Pds {

  class ZcpDatagramIterator : public InDatagramIterator {
  public:
    ZcpDatagramIterator( const ZcpDatagram& );
    ~ZcpDatagramIterator();

    void* operator new(unsigned,Pool*);
    void  operator delete(void*);

    int   skip(int len);                          // advance "len" bytes (without mapping)
    int   read(iovec* iov, int maxiov, int len);  // read (and map) "len" bytes into iov array and advance
    void* read_contiguous(int len, void* buffer);
  private:
    void _unmap_iovs();

    ZcpFragment _fragment;
    KStream     _stream;
    int         _size;
    int         _offset;  // only needed for debugging

    //
    //  A buffer of mapped data
    //
    class IovBuffer {
    public:
      IovBuffer();
      ~IovBuffer();
      
      void     unmap (KStream&);
      int      bytes () const { return _bytes; }     
      int      remove(int bytes);                    // advance over bytes (skip)
      int      remove(iovec*,int maxiov,int bytes);  // map bytes into iovec and advance
      int      insert(KStream&, int bytes);      // add bytes of fragment to buffer
    private:
      enum { MAXIOVS = 4096 }; // maximum iovs in one datagram
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
