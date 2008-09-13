#include "ZcpDatagram.hh"
#include "ZcpDatagramIterator.hh"

#include "Datagram.hh"
#include "pds/utility/ToEb.hh"

using namespace Pds;

ZcpDatagram::~ZcpDatagram()
{
  ZcpFragment* empty = _fragments.empty();
  ZcpFragment* fragm = _fragments.forward();
  while( fragm != empty ) {
    delete fragm->disconnect();
    fragm = _fragments.forward();
  }
}

const Datagram& ZcpDatagram::datagram() const
{
  return _datagram;
}

InDatagramIterator* ZcpDatagram::iterator(Pool* pool) const
{
  return new(pool) ZcpDatagramIterator(*this, pool);
}

int ZcpDatagram::send(ToEb& client, const Ins& dst)
{
  return client.send(this, dst);
}
