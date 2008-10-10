#include "ZcpDatagram.hh"
#include "ZcpDatagramIterator.hh"

#include "Datagram.hh"
#include "pds/utility/ToEb.hh"
#include "pds/utility/ToNetEb.hh"
#include "pds/service/OobServer.hh"

using namespace Pds;


ZcpDatagram::~ZcpDatagram()
{
}

const Datagram& ZcpDatagram::datagram() const
{
  return _datagram;
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

