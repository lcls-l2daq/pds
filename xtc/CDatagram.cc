#include "CDatagram.hh"
#include "CDatagramIterator.hh"

#include "pds/utility/ToEb.hh"

using namespace Pds;

CDatagram::~CDatagram()
{
}

const Datagram& CDatagram::datagram() const
{
  return _datagram;
}

int CDatagram::send(ToEb& client, const Ins& dst)
{
  return client.send(this, dst);
}

InDatagramIterator* CDatagram::iterator(Pool* pool) const
{
  return new (pool) CDatagramIterator(_datagram);
}
