#ifndef Pds_ChunkIterator_hh
#define Pds_ChunkIterator_hh

/*
**  Iterators to encapsulate the chunking policy
*/
#include "OutletWireHeader.hh"
#include "Mtu.hh"
#include "pds/xtc/Datagram.hh"

#include "pds/service/ZcpFragment.hh"
#include "pds/service/ZcpStream.hh"

#include <sys/socket.h>
#include <net/route.h>

#define HDR_POOL   Pds::Pool
#define HDR_POOL_E Pds::HeapEntry

//#define TEST_INCOMPLETE_CONTRIBUTIONS

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
    unsigned size(dg->xtc.sizeofPayload()+sizeof(Xtc));
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

  unsigned next() {
    _payload += _nextSize;
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
#ifdef TEST_INCOMPLETE_CONTRIBUTIONS
    --_remaining;
    return 0;
#else
    return --_remaining;
#endif
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
  ZcpChunkIterator(ZcpDatagram* datagram,
		   ZcpStream&   stream,
		   ZcpFragment& fragment) :
    _header(const_cast<Datagram*>(&datagram->datagram())),
    _stream  (stream),
    _fragment(fragment)
  {
    _nextSize = (_fragment.size()+sizeof(OutletWireHeader)+sizeof(Xtc) < Mtu::Size) ?
      _fragment.size() : Mtu::Size - sizeof(OutletWireHeader) - sizeof(Xtc);
    _remaining = datagram->datagram().xtc.sizeofPayload() - _fragment.size();
  }
  ~ZcpChunkIterator() {}

  const OutletWireHeader* header() { return &_header; }
  ZcpFragment& payload    ()       { return _fragment; }
  unsigned     payloadSize() const { return _nextSize; }

  bool next() {
    if (_remaining || _fragment.size()) {
      _header.offset = _header.length - _remaining - _fragment.size();
      _remaining    -= _stream.remove(_fragment,_remaining);
      _nextSize      = (_fragment.size() + sizeof(OutletWireHeader) < Mtu::Size) ?
	_fragment.size() : Mtu::Size - sizeof(OutletWireHeader);
      return true;
    }
    return false;
  }

private:
  OutletWireHeader  _header;
  ZcpStream&        _stream;
  ZcpFragment&      _fragment;
  unsigned          _nextSize;
  int               _remaining;
};
}
#endif
