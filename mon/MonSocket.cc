#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "pds/mon/MonSocket.hh"
#include "pds/service/Ins.hh"

using namespace Pds;

MonSocket::MonSocket(int socket) : 
  _socket(socket)
{
  _hdr.msg_name       = 0;
  _hdr.msg_namelen    = 0;
  _hdr.msg_control    = 0;
  _hdr.msg_controllen = 0;
  _hdr.msg_flags      = 0;
}

MonSocket::~MonSocket()
{
  if (_socket > 0) ::close(_socket);
}

int MonSocket::socket() const {return _socket;}

int MonSocket::read(void* buffer, int size)
{
  iovec iov;
  iov.iov_base = buffer;
  iov.iov_len  = size;
  return readv(&iov,1);
}

int MonSocket::write(const void* data, int size)
{
  iovec iov;
  iov.iov_base = const_cast<void*>(data);
  iov.iov_len  = size;
  return writev(&iov,1);
}

#include "pds/service/Sockaddr.hh"

int MonSocket::writev(const iovec* iov, int iovcnt)
{
  _hdr.msg_iov          = const_cast<iovec*>(iov);
  _hdr.msg_iovlen       = iovcnt;
  return ::sendmsg(_socket, &_hdr, MSG_DONTROUTE);
}

int MonSocket::setsndbuf(unsigned size) 
{
  if (::setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, 
		   (char*)&size, sizeof(size)) < 0) {
    return -1;
  }
  return 0;
}

int MonSocket::setrcvbuf(unsigned size) 
{
  if (::setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, 
		   (char*)&size, sizeof(size)) < 0) {
    return -1;
  }
  return 0;
}

int MonSocket::close()
{
  if (::close(_socket) < 0) {
    return -1;
  } else {
    _socket = -1;
  }
  return 0;
}
