#include "CDatagram.hh"
#include "CDatagramIterator.hh"

#include "pds/utility/TrafficDst.hh"
#include "pds/utility/ToNetEb.hh"
#include "pds/utility/ToEb.hh"
#include "pds/service/OobServer.hh"

#include <string.h>

using namespace Pds;

CDatagram::~CDatagram()
{
}

bool CDatagram::insert(const Xtc& tc, const void* payload)
{
  int payloadsiz = tc.extent - sizeof(Xtc);
  memcpy(xtc.alloc(sizeof(Xtc)), &tc, sizeof(Xtc));
  memcpy(xtc.alloc(payloadsiz), payload, payloadsiz);
  xtc.damage.increase(tc.damage.value());
  return true;
}

int CDatagram::send(ToEb& client)
{
  return client.send(this);
}

int CDatagram::send(ToNetEb& client, const Ins& dst)
{
  return client.send(this, dst);
}

int CDatagram::unblock(OobServer& srv, char* msg)
{
  //  We need to reconstitute the vtable at the other end of the unblock.
  return srv.unblock(msg, const_cast<char*>(reinterpret_cast<const char*>(this)),
		     xtc.sizeofPayload()+sizeof(*this));
}

InDatagramIterator* CDatagram::iterator(Pool* pool) const
{
  return new (pool) CDatagramIterator(*this);
}

TrafficDst* CDatagram::traffic(const Ins& dst)
{
  return new CTraffic(this, dst);
}
