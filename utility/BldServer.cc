#include "BldServer.hh"

using namespace Pds;

BldServer::BldServer(const Ins& ins,
		     const Src& src,
		     unsigned   nbufs) :
  NetDgServer(ins, src, nbufs) {}

BldServer::~BldServer() {}

bool BldServer::isValued() const
{
  return false;
}
