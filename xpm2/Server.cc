#include "pds/xpm2/Server.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/Dgram.hh"

using namespace Pds;

Xpm2::Server::Server(Module&    dev,
                    const Src& src) :
  _dev (dev),
  _xtc (_xtcType,src)
{
}

Xpm2::Server::~Server()
{
}

int Xpm2::Server::fetch(char* payload,
                       int   flags)
{
  return -1;
}
