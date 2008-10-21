#include "ZcpDatagramIterator.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace Pds;

ZcpDatagramIterator::ZcpDatagramIterator( const ZcpDatagram& dg) :
  _size  (dg.datagram().xtc.sizeofPayload()),
  _offset(0)
{
  ZcpFragment  zcp;
  ZcpDatagram& zdg(const_cast<ZcpDatagram&>(dg));
  int remaining(_size);
  while( remaining ) {
    int len = zdg._stream.remove(_fragment, remaining);
    zcp.copy(_fragment, len);
    int err;
    if ( (err = zdg._stream.insert(_fragment, len)) != len ) {
      if (err < 0)
	printf("ZDI error scanning input stream : %s\n", strerror(errno));
      else if (err != len)
	printf("ZDI error scanning input stream %d/%d bytes\n", err,len);
    }
    if ( (err = _stream.insert(zcp, len)) != len ) {
      if (err < 0)
	printf("ZDI error copying input stream : %s\n", strerror(errno));
      else if (err != len)
	printf("ZDI error copying input stream %d/%d bytes\n", err,len);
    }
    remaining -= len;
  }
}

ZcpDatagramIterator::~ZcpDatagramIterator()
{
  _iovbuff.unmap(_stream);
}

//
// advance "len" bytes (without mapping)
//
int ZcpDatagramIterator::skip(int len)
{
  if (_offset + len > _size +_iovbuff.bytes()) {
    len = _size - _offset + _iovbuff.bytes();
  }

  int remaining(len);
  // skip over mapped bytes first
  remaining -= _iovbuff.remove(len);  

  // skip over unmapped bytes
#if 0
  _stream.remove(remaining);
#else
  while (remaining > _iovbuff.bytes())
    _iovbuff.insert(_stream, remaining-_iovbuff.bytes());
  remaining -= _iovbuff.remove(remaining);
#endif

  _offset += (len-remaining);

  return len-remaining;
}

//
// read (and map) "len" bytes into iov array and advance
//
int ZcpDatagramIterator::read(iovec* iov, int maxiov, int len)
{
  if (_offset + len > _size + _iovbuff.bytes()) {
    len = _size - _offset + _iovbuff.bytes();
  }

  //  Pull enough data into the IovBuffer
  while (len > _iovbuff.bytes()) {
    _offset += _iovbuff.insert(_stream, len-_iovbuff.bytes());
  }

  //  Map the data into user-space and return the user iov
  return _iovbuff.remove(iov,maxiov,len);
}


ZcpDatagramIterator::IovBuffer::IovBuffer() :
  _bytes  (0),
  _iiov   (0),
  _iiovlen(0),
  _niov   (0)
{
  _iovs[_iiov].iov_len = 0;
}

ZcpDatagramIterator::IovBuffer::~IovBuffer()
{
}

void ZcpDatagramIterator::IovBuffer::unmap(KStream& stream)
{
  stream.unmap(_iovs, _niov);
}

//
//  Here we are skipping over buffers that were already mapped
//
int ZcpDatagramIterator::IovBuffer::remove(int bytes)
{
  int len(bytes);
  if (_iiovlen) {
    len -= _iiovlen;
    _iiovlen = 0;
    _iiov++;
  }
  while( len>0 && _iiov<_niov ) {
    len -= _iovs[_iiov].iov_len;
    _iiov++;
  }
  if (len < 0) {
    _iiovlen = -len;
    _iiov--;
    len = 0;
  }
  bytes  -= len;
  _bytes -= bytes;
  return bytes;
}

//
//  Pass these mapped buffers to the user's iovs
//
int ZcpDatagramIterator::IovBuffer::remove(iovec* iov, int maxiov, int bytes)
{
  iovec* iovend = iov+maxiov;
  int len = bytes;
  if (_iiovlen) {
    iov->iov_base = (char*)_iovs[_iiov].iov_base + 
      _iovs[_iiov].iov_len - _iiovlen;
    iov->iov_len = _iiovlen;
    len -= _iiovlen;
    _iiovlen = 0;
    _iiov++;
    iov++;
  }
  while( len>0 && iov<iovend ) {
    iov->iov_base = _iovs[_iiov].iov_base;
    iov->iov_len  = _iovs[_iiov].iov_len;
    len          -= _iovs[_iiov].iov_len;
    iov++;
    _iiov++;
  }
  if (len < 0) {
    _iiovlen = -len;
    _iiov--;
    iov--;
    iov->iov_len += len;
    len = 0;
  }
  bytes  -= len;
  _bytes -= bytes;
  return bytes;
}

//
//  Map more data into the iovs
//
int ZcpDatagramIterator::IovBuffer::insert(KStream& stream, int bytes)
{
  int obytes(_bytes);

  do {
    //  Try the minimum number of iovs (4kB page size)
    int niov = (bytes >> 12)+1;
    if (_niov+niov > MAXIOVS)
      niov = MAXIOVS - _niov;

    niov = stream.map(&_iovs[_niov],niov);
    if (niov < 0) {
      printf("ZDI::IovBuffer::insert kmem_read failed %s(%d) : iov %p/%d maxiov %d\n",
	     strerror(errno),errno,&_iovs[_niov],_niov,MAXIOVS-_niov);
      break;
    }

    int len  = 0;
    while( niov-- )
      len += _iovs[_niov++].iov_len;
    if (_niov == MAXIOVS)
      printf("ZcpDatagramIterator::IovBuffer exhausted %d iovs\n",_niov);
    bytes  -= len;
    _bytes += len;
  } while (bytes > 0);

  return _bytes-obytes;
}
