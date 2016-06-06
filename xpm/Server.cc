#include "pds/xpm/Server.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/Dgram.hh"

using namespace Pds;

Xpm::Server::Server(Module&    dev,
                    const Src& src) :
  _dev (dev),
  _xtc (_xtcType,src)
{
}

Xpm::Server::~Server()
{
}

int Xpm::Server::fetch(char* payload,
                       int   flags)
{
  return -1;
}
