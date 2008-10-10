#include "CDatagram.hh"
#include "CDatagramIterator.hh"

using namespace Pds;

CDatagramIterator::~CDatagramIterator()
{
}

int CDatagramIterator::skip(int len)
{
  int qlen = _offset+len > _end ? _end-_offset : len;
  _offset += qlen;
  return qlen;
}

int CDatagramIterator::read(iovec* iov, int maxiov, int len)
{
  int qlen = _offset+len > _end ? _end-_offset : len;
  iov[0].iov_base = (char*)&_datagram + _offset;
  iov[0].iov_len  = qlen;
  _offset += qlen;
  return qlen;
}
