#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "pds/mon/MonSocket.hh"
#include "pds/service/Ins.hh"
#include "pds/mon/MonSocketAddress.hh"

using namespace Pds;

MonSocket::MonSocket() : _socket(-1) {}

MonSocket::MonSocket(int socket) : _socket(socket) {}

MonSocket::~MonSocket()
{
  if (_socket > 0) ::close(_socket);
}

int MonSocket::socket() const {return _socket;}

int MonSocket::read(void* buffer, int size) const
{
  return ::recv(_socket, buffer, size, MSG_WAITALL);
}

int MonSocket::write(const void* data, int size) const
{
  return ::send(_socket, data, size, MSG_DONTROUTE);
}

int MonSocket::readv(const iovec* iov, int iovcnt) const
{
  struct msghdr hdr;
  hdr.msg_name         = 0;
  hdr.msg_namelen      = 0;
  hdr.msg_iov          = const_cast<iovec*>(iov);
  hdr.msg_iovlen       = iovcnt;
  hdr.msg_control      = 0;
  hdr.msg_controllen   = 0;
  return ::recvmsg(_socket, &hdr, MSG_WAITALL);
}

int MonSocket::writev(const iovec* iov, int iovcnt) const
{
  struct msghdr hdr;
  hdr.msg_name         = 0;
  hdr.msg_namelen      = 0;
  hdr.msg_iov          = const_cast<iovec*>(iov);
  hdr.msg_iovlen       = iovcnt;
  hdr.msg_control      = 0;
  hdr.msg_controllen   = 0;
  return ::sendmsg(_socket, &hdr, MSG_DONTROUTE);
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

int MonSocket::connect(const Ins& dst)
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }
  MonSocketAddress sa(dst);
  if (::connect(_socket, sa.name(), sa.sizeofName()) < 0) {
    close();
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

int MonSocket::listen(const Ins& src)
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }
  int optval=1;
  if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    close();
    return -1;
  }
  MonSocketAddress sa(src);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0) {
    close();
    return -1;
  }
  if (::listen(_socket, 5) < 0) {
    close();
    return -1;
  }
  return 0;
}

int MonSocket::accept()
{
  sockaddr_in name;
  unsigned length = sizeof(name);
  return ::accept(_socket, (sockaddr*)&name, &length);
}

static const int LoopbackAddr = 0x7f000001;

int MonSocket::loopback()
{
  if ((_socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return -1;
  }
  Ins loopback(LoopbackAddr);
  MonSocketAddress sa(loopback);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0) {
    close();
    return -1;
  }
  socklen_t addrlen = sizeof(sockaddr_in);
  sockaddr_in name;
  if (::getsockname(_socket, (sockaddr*)&name, &addrlen) < 0) {
    close();
    return -1;
  }
  if (::connect(_socket, (const sockaddr*)&name, addrlen) < 0) {
    close();
    return -1;
  }
  return 0;
}
