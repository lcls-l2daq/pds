/*
**  Iterators to encapsulate the chunking policy
*/
#include "OutletWireHeader.hh"
#include "Mtu.hh"
#include "pds/xtc/Datagram.hh"

#include <sys/socket.h>
#include <net/route.h>

#define HDR_POOL   Pds::Pool
#define HDR_POOL_E Pds::HeapEntry

namespace Pds {
/*
**  A chunk iterator for a contiguous datagram
*/
class DgChunkIterator {
public:
  DgChunkIterator(const Datagram* dg)
    : _header(dg),
      _payload((char*)&dg->xtc)
  {
    unsigned size(dg->xtc.sizeofPayload()+sizeof(InXtc));
    _nextSize = size & Mtu::SizeAsMask;
    _remaining = (size >> Mtu::SizeAsPowerOfTwo)+1;
    
    if (!_nextSize) {
      _nextSize=Mtu::Size;
      _remaining--;
    }
  }
  ~DgChunkIterator() {}

  const OutletWireHeader* header() { return &_header; }
  const char* payload() { return _payload; }
  unsigned payloadSize() { return _nextSize; }

  unsigned last() {
    return _remaining==1; 
  }

  unsigned next() {
    _payload += _nextSize;
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
    return --_remaining;
  }

private:
  OutletWireHeader  _header;
  char*                _payload;
  unsigned             _nextSize;
  int                  _remaining;
};

/*
**  This iterator alters the array it is passed to present the chunk iovecs for sending.
*/
class IovChunkIterator {
public:
  IovChunkIterator(struct iovec*& iovArray,unsigned& iovCount) :
    _iovArray(iovArray),
    _iovCount(iovCount),
    _iov(iovArray),
    _header((Datagram*)iovArray[0].iov_base),
    _payload((char*)&_header.damage) 
  {
    Datagram* dg = (Datagram*)iovArray[0].iov_base;
    unsigned size = dg->xtc.sizeofPayload()+sizeof(InXtc);
    _nextSize = size & Mtu::SizeAsMask;
    _remaining = (size >> Mtu::SizeAsPowerOfTwo)+1;
    
    if (!_nextSize) {
      _nextSize=Mtu::Size;
      _remaining--;
    }

    advance();
  }
  ~IovChunkIterator() {}

  const OutletWireHeader* header() { return &_header; }

  unsigned iovCount() { return _iovCount; }

  struct iovec* iovArray() { return _iovArray; }

  unsigned last() {
    return _remaining==1; 
  }

  unsigned next() {
    if (_iov_len) { // Reattach the piece that didn't fit
      _iov->iov_base = _iov_base;
      _iov->iov_len  = _iov_len;
      --_iov;
    }

    if (--_remaining) {
      _header.offset += _nextSize;
      _nextSize = Mtu::Size;
      _iovArray = _iov;
      advance();
    }
    return _remaining;
  }

private:
  void advance() {
    int sum = _nextSize;
    do sum -= (++_iov)->iov_len; while (sum>0);
    _iovCount = (_iov+1) - _iovArray;
    _iov_len = 0;
    if (sum) {  // Cut off the piece that doesn't fit
      _iov_len       = -sum;
      _iov->iov_len += sum;
      _iov_base      = (char*)_iov->iov_base + _iov->iov_len;
    }
  }
private:
  struct iovec*        _iovArray;
  unsigned             _iovCount;
  struct iovec*        _iov;
  caddr_t              _iov_base;
  size_t               _iov_len;
  OutletWireHeader  _header;
  char*                _payload;
  unsigned             _nextSize;
  int                  _remaining;
};

/*
**  This class is responsible for providing UDP chunk headers.
*/

class AsyncIterator {
public:
  AsyncIterator( const OutletWireHeader& header,
		    const Datagram* dg,
		    HDR_POOL& pool)
    : _header(header),
      _pool(pool) {}
  ~AsyncIterator() {}

  OutletWireHeader* header() {
    OutletWireHeader* rval  = new (&_pool) OutletWireHeader(_header);
    rval->offset = _header.offset;
    return rval;
  }

protected:
  const OutletWireHeader& _header;
  HDR_POOL& _pool;
};

/*
**  A chunk iterator for a contiguous datagram
*/
class AsyncDgChunkIterator : public DgChunkIterator,
				public AsyncIterator {
public:
  AsyncDgChunkIterator(const Datagram* dg,
			  HDR_POOL& headers) :
    DgChunkIterator(dg),
    AsyncIterator(*DgChunkIterator::header(), dg, headers) {}

  ~AsyncDgChunkIterator() {}

  OutletWireHeader* header() { return AsyncIterator::header(); }
};

/*
**  A chunk iterator for an indirect datagram
*/
class AsyncIovChunkIterator : public AsyncIterator,
				 public IovChunkIterator {
public:
  AsyncIovChunkIterator(struct iovec*& iovArray,unsigned& iovCount,
			   HDR_POOL& headers) :
    AsyncIterator(*IovChunkIterator::header(),
		     (Datagram*)iovArray[0].iov_base,
		     headers),
    IovChunkIterator(iovArray,iovCount) {}

  ~AsyncIovChunkIterator() {}

  OutletWireHeader* header() { return AsyncIterator::header(); }
};
}
