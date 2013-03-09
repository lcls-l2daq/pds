#include "pds/utility/BldServerTransient.hh"

#include "pds/xtc/XtcType.hh"

using namespace Pds;

BldServerTransient::BldServerTransient(const Ins& ins,
				       const Src& src,
				       unsigned   maxbuf) :
  BldServer(ins, src, maxbuf),
  _xtc(0)
{
}

BldServerTransient::~BldServerTransient()
{
}

const Xtc&   BldServerTransient::xtc   () const
{
  return *_xtc;
}

unsigned BldServerTransient::length() const
{
  return BldServer::length()+sizeof(Xtc);
}

unsigned BldServerTransient::offset() const
{
  unsigned o = BldServer::offset();
  return o ? o+sizeof(Xtc):0;
}

int BldServerTransient::fetch(char* payload, int flags)
{
  int length = 0;
  if (BldServer::offset()==0) {
    _xtc = new (payload) Xtc(_transientXtcType,
			     BldServer::client(),
			     BldServer::xtc().damage);
    _xtc->extent += BldServer::xtc().extent;
    length += sizeof(Xtc);
  }
  length += BldServer::fetch(payload+length, flags);
  return length;
}
