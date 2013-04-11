#include "NetDgServer.hh"
#include "EbEventBase.hh"
#include "Mtu.hh"
#include "OutletWireHeader.hh"

#include <stdio.h>

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

NetDgServer::NetDgServer(const Ins& ins,
			 const Src& src,
			 unsigned   maxbuf) :
  _server((unsigned)-1,
	  ins,
	  DatagramSize,
	  MaxPayload,
	  maxbuf / (MaxPayload+DatagramSize) + 1 ),
  _client(src)
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
  printf("Server %04X (%x/%d)  fd %d\n",
	 id(),
	 _server.address(),
	 _server.portId(),
	 _server.fd());
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

const Xtc&   NetDgServer::xtc   () const
{
  return ((Datagram*)_server.datagram())->xtc;
}

const Src& NetDgServer::client () const
{
  return _client;
}

bool     NetDgServer::more  () const
{
  return ((Datagram*)_server.datagram())->seq.isExtended();
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
  return length;
}

int NetDgServer::fetch(ZcpFragment& dg, int flags)
{
  int length = _server.fetch(dg,flags);
  return length;
}
