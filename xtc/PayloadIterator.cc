#include "PayloadIterator.hh"

using namespace Pds;

int PayloadIterator::skip(int len)
{
  int qlen = _offset+len > _end ? _end-_offset : len;
  _offset += qlen;
  return qlen;
}

int PayloadIterator::read(iovec* iov, int maxiov, int len)
{
  int qlen = _offset+len > _end ? _end-_offset : len;
  iov[0].iov_base = const_cast<char*>(_base) + _offset;
  iov[0].iov_len  = qlen;
  _offset += qlen;
  return qlen;
}

void* PayloadIterator::read_contiguous(int len, void* buffer)
{
  char* dg = const_cast<char*>(_base) + _offset;
  if (_offset+len > _end) return 0;
  _offset += len;
  return dg;
}
