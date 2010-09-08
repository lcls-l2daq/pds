#ifndef Pds_ChunkIterator_hh
#define Pds_ChunkIterator_hh

/*
**  Iterators to encapsulate the chunking policy
*/
#include "OutletWireHeader.hh"
#include "Mtu.hh"
#include "pds/xtc/Datagram.hh"

#include "pds/xtc/ZcpDatagram.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/service/ZcpStream.hh"

#include <sys/socket.h>
#include <net/route.h>

#define HDR_POOL   Pds::Pool
#define HDR_POOL_E Pds::HeapEntry

//#define DROP_FIRST
//#define DROP_AFTER_FIRST
//#define DROP_LAST
//#define DROP_MIDDLES
//#define GEN_MISS

#ifdef GEN_MISS
static Pds::OutletWireHeader _miss_header;
#endif

namespace Pds {
/*
**  A chunk iterator for a contiguous datagram
*/
class DgChunkIterator {
public:
  DgChunkIterator(const Datagram* dg)
    : _header (dg),
      _payload((const char*)&dg->xtc) 
  { _construct(dg); }

  DgChunkIterator(const Datagram* dg,
		  const char*     payload)
    : _header(dg),
      _payload(payload)
  { _construct(dg); }

  ~DgChunkIterator() {}

  const OutletWireHeader* header() { return &_header; }
  const char* payload() { return _payload; }
  unsigned payloadSize() { return _nextSize; }

  unsigned next() {
#ifdef DROP_AFTER_FIRST
    return 0;
#else
#ifdef DROP_MIDDLES
    if (_remaining < 2) return 0;
    while (--_remaining) {
      _payload += _nextSize;
      _header.offset += _nextSize;
      _nextSize = Mtu::Size;
    }
    return 1;
#else
#ifdef DROP_LAST
    if (_remaining==2) 
      return 0;

    _payload += _nextSize;
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
    return --_remaining;
#else
#ifdef GEN_MISS
    if (_remaining==2) {  // swap last chunk
      OutletWireHeader h = _header;
      _header = _miss_header;
      _miss_header = h;
    }
    _payload += _nextSize;
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
    return --_remaining;

#else  // drop nothing
    _payload += _nextSize;
    _header.offset += _nextSize;
    _nextSize = Mtu::Size;
    return --_remaining;
#endif
#endif
#endif
#endif
  }

private:
  void _construct(const Datagram* dg) {
    unsigned size(dg->xtc.sizeofPayload()+sizeof(Xtc));
    _nextSize = size & Mtu::SizeAsMask;
    _remaining = (size >> Mtu::SizeAsPowerOfTwo)+1;
    
    if (!_nextSize) {
      _nextSize=Mtu::Size;
      _remaining--;
    }
#ifdef DROP_FIRST
    next();
#endif
  }
private:
  OutletWireHeader  _header;
  const char*          _payload;
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
