#include "NetDgServer.hh"
#include "Mtu.hh"
#include "OutletWireHeader.hh"

using namespace Pds;

/*
** ++
**
**    This function instantiates a server. A server is created whenever
**    a client upstream of the server comes into existence. I.e., for
**    each client, their exists one and only one server. The client
**    has a identity ("id"), which is passed as an argument to the
**    constructor. This ID is a small dense integer varying (currently)
**    from zero to thirty-one (decimal), used to uniquely identify the
**    client. Within, the server the ID is represented as a bit mask,
**    allowing the object's which use the ID to operate more efficiently.
**    The constructor is passed two fixed-length pools:
**    "datagrams", which serves as a freelist for event buffering, and
**    "events" which serves as a freelist for the objects which describe
**    the context necessary to construct an event. The "pending" argument
**    is the list of events currently under either construction or completion.
**    The queue is in arrival order, with the oldest event at the head and
**    the youngest event at the tail. Since the socket paradigm forces the
**    implementation to allocate one ahead (the read interface requires
**    a buffer to read into), an event may be under "construction", which
**    means the event has been allocated but has not yet seen an actual
**    contribution. An event which has seen at least one contribution, but
**    not all its necessary contributions is said to be under "completion".
**    The "clients" argument points to the list of all currently known
**    clients. This function saves all the arguments and locates the event
**    to contain its first seen contribution.
**
**
** --
*/

static const unsigned DatagramSize = sizeof(Datagram);
static const unsigned MaxPayload = Mtu::Size;

NetDgServer::NetDgServer(const Src& client,
			 const Ins& ins,
			 unsigned   nbufs) :
  EbServer (client, client.pid(), 0),
  _server(client.pid(),
	  ins,
	  DatagramSize,
	  MaxPayload,
	  nbufs)
{
  fd(_server.fd());
}

/*
** ++
**
**    Simple diagnostic tool to dump the characterization of this server...
**
** --
*/

void NetDgServer::dump(int detail) const
  {
  printf("Server %04X (port %d) represents client %04X/%04X (did/pid)\n",
         id(),
         _server.portId(),
         client().did(),
         client().pid());
  printf(" %d Network buffers of %d bytes each (%d+%d bytes header+payload)\n",
         _server.maxDatagrams(),
         _server.sizeofDatagram() + _server.maxPayload(),
         _server.sizeofDatagram(), _server.maxPayload());
  printf(" Dropped %d contributions\n", drops());
  return;
  }

/*
** ++
**
**    Destructor for server.
**
** --
*/

NetDgServer::~NetDgServer()
{
}



bool NetDgServer::isValued() const
{
  return true;
}

unsigned NetDgServer::keyTypes() const
{
  return (1 << EbKey::PulseId);
}

const char* NetDgServer::key(EbKey::Type type) const
{
  if (type==EbKey::PulseId)
    return reinterpret_cast<const char*>(&_pulseId);
  return 0;
}

bool NetDgServer::matches(const EbKey& keys) const
{
  if (!keys.types & (1<<EbKey::PulseId))
    return false;
  return keys.pulseId==_pulseId;
}

const InXtc&   NetDgServer::xtc   () const
{
  return ((Datagram*)_server.datagram())->xtc;
}

bool     NetDgServer::more  () const
{
  return ((Datagram*)_server.datagram())->isExtended();
}

unsigned NetDgServer::length() const
{
  return ((OutletWireHeader*)_server.datagram())->length;
}

unsigned NetDgServer::offset() const
{
  return ((OutletWireHeader*)_server.datagram())->offset;
}

int NetDgServer::pend(int flag)
{
  return _server.pend(flag);
}

#include <stdio.h>

int NetDgServer::fetch(char* payload, int flags)
{
  int length = _server.fetch(payload,flags);
  _pulseId = *(Sequence*)_server.datagram();
  return length;
}

int NetDgServer::fetch(ZcpFragment& dg, int flags)
{
  int length = _server.fetch(dg,flags);
  _pulseId = *(Sequence*)_server.datagram();
  return length;
}
