#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "pds/mon/MonStreamSocket.hh"
#include "pds/mon/MonMessage.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"

using namespace Pds;

MonStreamSocket::MonStreamSocket() {}

MonStreamSocket::MonStreamSocket(int socket) : MonSocket(socket) {}

MonStreamSocket::~MonStreamSocket()
{
}

int MonStreamSocket::fetch(MonMessage* msg)
{
  return read((char*)msg, sizeof(*msg));
}

int MonStreamSocket::readv(const iovec* iov, int iovcnt)
{
  _hdr.msg_iov          = const_cast<iovec*>(iov);
  _hdr.msg_iovlen       = iovcnt;
  return ::recvmsg(_socket, &_hdr, MSG_WAITALL);
}

int MonStreamSocket::connect(const Ins& dst)
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }
  Sockaddr sa(dst);
  if (::connect(_socket, sa.name(), sa.sizeofName()) < 0) {
    close();
    return -1;
  }
  return 0;
}

int MonStreamSocket::listen(const Ins& src)
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }
  int optval=1;
  if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    close();
    return -1;
  }
  Sockaddr sa(src);
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

int MonStreamSocket::accept()
{
  sockaddr_in name;
  unsigned length = sizeof(name);
  return ::accept(_socket, (sockaddr*)&name, &length);
}

