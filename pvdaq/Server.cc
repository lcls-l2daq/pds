#include "pds/pvdaq/Server.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::PvDaq;

Server::Server() : _env(0)
{
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    fd(_pfd[0]);
  }
}

int Server::fetch( char* payload, int flags )
{
  void* p;
  int len = ::read(_pfd[0], &p, sizeof(p));
  if (len != sizeof(p)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  return fill(payload,p);
}

void Server::post(const void* p)
{
  ::write(_pfd[1], &p, sizeof(p));
}


#include "pds/pvdaq/BeamMonitorServer.hh"

Server* Server::lookup(const char*    pvbase,
                       const DetInfo& info)
{
  Server* s=0;
  switch(info.device()) {
  case Pds::DetInfo::Wave8:
    s = new BeamMonitorServer(pvbase,info);
    break;
  default:
    break;
  }
  return s;
}
