#include "Server.hh"
#include "pds/config/UsdUsbDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::UsdUsb;

Server::Server( const Src& client )
  : _xtc( _usdusbDataType, client )
{
  _xtc.extent = sizeof(UsdUsbDataType) + sizeof(Xtc);
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    fd(_pfd[0]);
  }
}

int Server::fetch( char* payload, int flags )
{
  Xtc& xtc = *reinterpret_cast<Xtc*>(payload);
  memcpy(payload, &_xtc, sizeof(Xtc));

  int len = ::read(_pfd[0], xtc.payload(), sizeof(UsdUsbDataType));
  if (len != sizeof(UsdUsbDataType)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  _count++;
  return xtc.extent;
}

unsigned Server::count() const
{
  return _count - 1;
}

void Server::resetCount()
{
  _count = 0;
}

void Server::post(char* payload, unsigned sz)
{
  ::write(_pfd[1], payload, sz);
}
