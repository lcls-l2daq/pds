#ifndef Pds_ChunkIterator_hh
#define Pds_ChunkIterator_hh

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
class ZcpChunkIterator {
public:
  ZcpChunkIterator(Datagram* datagram) :
    _header(datagram)
  {
    unsigned size = datagram->xtc.sizeofPayload()+sizeof(InXtc);
    _nextSize = size & Mtu::SizeAsMask;
    _remaining = (size >> Mtu::SizeAsPowerOfTwo)+1;
    
    if (!_nextSize) {
      _nextSize=Mtu::Size;
      _remaining--;
    }
  }
  ~ZcpChunkIterator() {}

  const OutletWireHeader* header() { return &_header; }
  unsigned payloadSize() const { return _nextSize; }

  unsigned last() {
    return _remaining==1; 
  }

  unsigned next() {
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
    return --_remaining;
  }

private:
  OutletWireHeader  _header;
  unsigned          _nextSize;
  int               _remaining;
};
}
#endif
