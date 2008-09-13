#include "ZcpDatagramIterator.hh"

#include "ZcpDatagram.hh"

using namespace Pds;

ZcpDatagramIterator::ZcpDatagramIterator( const ZcpDatagram& dg,
					  Pool* pool) :
  _datagram(dg.datagram())
{
  ZcpFragment* empty = dg.fragments().empty();
  ZcpFragment* zfrag = dg.fragments().forward();
  while( zfrag != empty ) {
    _fragments.insert( new(pool) ZcpFragment(*zfrag) );
    zfrag = zfrag->forward();
  }
}

ZcpDatagramIterator::~ZcpDatagramIterator()
{
  //  unmap the outstanding iovs
  _iovbuff.unmap();
  //  delete the remaining fragments
  ZcpFragment* empty = _fragments.empty();
  ZcpFragment* zfrag;
  while( (zfrag=_fragments.forward()) != empty )
    delete zfrag->disconnect();
}

int ZcpDatagramIterator::skip(int len)
{
  while (len > _iovbuff.bytes()) {
    ZcpFragment* zfrag = _fragments.forward();
    _iovbuff.insert(*zfrag,_iovbuff.bytes()-len);
    delete zfrag->disconnect();
  }
  _iovbuff.remove(len);
  return len;
}

int ZcpDatagramIterator::read(iovec* iov, int maxiov, int len)
{
  while (len > _iovbuff.bytes()) {
    ZcpFragment* zfrag = _fragments.forward();
    _iovbuff.insert(*zfrag, len-_iovbuff.bytes());
    delete zfrag->disconnect();
  }
  return _iovbuff.remove(iov,maxiov,len);
}

int ZcpDatagramIterator::copy(void* buff, int len)
{
  ZcpFragment zfrag;
  int klen = len;
  ZcpFragment* from = _fragments.forward();
  while( klen ) {
    klen -= zfrag.copyFrom(*from,klen);
    from = from->forward();
  }
  return zfrag.read(buff,len);
}

ZcpDatagramIterator::IovBuffer::IovBuffer() :
  _bytes(0),
  _iiovlen(0)
{
  _iovs[_iiov=0].iov_len = 0;
}

ZcpDatagramIterator::IovBuffer::~IovBuffer()
{
  unmap();
}

void ZcpDatagramIterator::IovBuffer::unmap()
{
}

//
//  Here we would like to skip over buffers without having to map/unmap them.
//
int ZcpDatagramIterator::IovBuffer::remove(int bytes)
{
  int len = bytes;
  if (_iiovlen) {
    len -= _iiovlen;
    _iiovlen = 0;
    _iiov++;
  }
  while( len>0 ) {
    len -= _iovs[_iiov].iov_len;
    _iiov++;
  }
  if (len < 0) {
    _iiovlen = -len;
    _iiov--;
  }
  _bytes -= bytes;
  return bytes;
}

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
  }
  _bytes -= bytes;
  return bytes;
}

int ZcpDatagramIterator::IovBuffer::insert(ZcpFragment& zfrag, int bytes)
{
  int len = zfrag.vremove(&_iovs[_niov],MAXIOVS-_niov);
  int qlen(len);
  while (qlen)
    qlen -= _iovs[_niov++].iov_len;
  _bytes += len;
  return len;
}
