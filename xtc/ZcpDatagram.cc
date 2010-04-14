#include "ZcpDatagram.hh"
#include "ZcpDatagramIterator.hh"

#include "Datagram.hh"
#include "pds/utility/ToEb.hh"
#include "pds/utility/ToNetEb.hh"
#include "pds/utility/TrafficDst.hh"
#include "pds/service/OobServer.hh"

using namespace Pds;


ZcpDatagram::~ZcpDatagram()
{
}

bool ZcpDatagram::insert(const Xtc& tc, const void* payload)
{
  ZcpFragment frag;
  _stream.insert(frag,frag.uinsert(&tc,sizeof(Xtc)));

  int remaining = tc.extent - sizeof(Xtc);
  const char* p = reinterpret_cast<const char*>(payload);
  while(remaining) {
    int len = frag.uinsert(p, remaining);
    if (len < 0)
      return false;
    if (_stream.insert(frag,len)!=len)
      return false;
    p         += len;
    remaining -= len;
  }
  xtc.extent += tc.extent;
  return true;
}

InDatagramIterator* ZcpDatagram::iterator(Pool* pool) const
{
  return new(pool) ZcpDatagramIterator(*this);
}

int ZcpDatagram::send(ToEb& client)
{
  return client.send(this);
}

int ZcpDatagram::send(ToNetEb& client, const Ins& dst)
{
  return client.send(this, dst);
}

int ZcpDatagram::unblock(OobServer& srv, char* msg)
{
  /*
  return srv.unblock(msg, const_cast<char*>(reinterpret_cast<const char*>(&_datagram)),
		     _datagram.xtc.sizeofPayload()+sizeof(Datagram),
		     _fragments);
  */
  return 0;
}

TrafficDst* ZcpDatagram::traffic(const Ins& ins)
{
  return new ZcpTraffic(this, ins);
}
