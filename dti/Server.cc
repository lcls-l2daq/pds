#include "pds/dti/Server.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/Dgram.hh"

using namespace Pds;

Dti::Server::Server(Module&    dev,
                    const Src& src) :
  _dev (dev),
  _xtc (_xtcType,src)
{
}

Dti::Server::~Server()
{
}

int Dti::Server::fetch(char* payload,
                       int   flags)
{
  return -1;
}
