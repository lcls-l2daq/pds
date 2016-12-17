#include "Server.hh"
#include "pds/config/ArchonDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::Archon;

Server::Server( const Src& client )
  : _xtc( _archonDataType, client ), _count(0), _framesz(0)
{
  _xtc.extent = sizeof(ArchonDataType) + sizeof(Xtc);
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

  int len = ::read(_pfd[0], xtc.payload(), sizeof(ArchonDataType) + _framesz);
  if (len != (int) (sizeof(ArchonDataType) + _framesz)) {
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

void Server::set_frame_sz(unsigned sz)
{
  _framesz = sz;
  _xtc.extent = sizeof(ArchonDataType) + sizeof(Xtc) + sz;
}
