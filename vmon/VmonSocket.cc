#include "pds/vmon/VmonSocket.hh"

#include "pds/mon/MonMessage.hh"
#include "pds/service/Ins.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace Pds;

VmonSocket::VmonSocket() :
  _iovs(new iovec[_iovcnt=10]),
  _peeked(false)
{
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;
}

VmonSocket::~VmonSocket()
{
}

int VmonSocket::readv(const iovec* iov, int iovcnt)
{
  bool peeked = _peeked;
  if (_peeked) {
    if (iovcnt >= _iovcnt) {
      iovec iovhdr = _iovs[0];
      delete[] _iovs;
      _iovs = new iovec[_iovcnt=iovcnt+1];
      _iovs[0] = iovhdr;
    }
    for(int i=0; i<iovcnt; i++)
      _iovs[i+1] = iov[i];
    _rhdr.msg_iov    = _iovs;
    _rhdr.msg_iovlen = iovcnt+1;
    _peeked = false;
  }
  else {
    _rhdr.msg_iov    = const_cast<iovec*>(iov);
    _rhdr.msg_iovlen = iovcnt;
  }
  int bytes = ::recvmsg(_socket, &_rhdr, 0);
  return bytes;
}

int VmonSocket::peek(MonMessage* msg)
{
  _peeked = true;
  _iovs[0].iov_base  = (char*)msg;
  _iovs[0].iov_len   = sizeof(*msg);
  _rhdr.msg_iov    = _iovs;
  _rhdr.msg_iovlen = 1;
  int bytes = ::recvmsg(_socket, &_rhdr, MSG_PEEK);
  return bytes;
}

int VmonSocket::flush()
{
  if (_peeked) {
    _rhdr.msg_iov    = _iovs;
    _rhdr.msg_iovlen = 1;
    _peeked = false;
    return ::recvmsg(_socket, &_rhdr, 0);
  }
  return 0;
}
