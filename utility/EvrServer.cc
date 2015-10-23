#include "pds/utility/EvrServer.hh"
#include "pds/utility/EbEventBase.hh"
#include "pds/utility/Mtu.hh"
#include "pds/utility/Inlet.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/xtc/EvrDatagram.hh"

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

static const unsigned DatagramSize = sizeof(EvrDatagram);
static const unsigned MaxPayload = Mtu::Size;

EvrServer::EvrServer(const Ins& ins,
         const Src& src,
                     Inlet&     inlet,
         unsigned   nbufs) :
  _server ((unsigned)-1,
           Port::VectoredServerPort,  // REUSE_ADDR
           ins,
           DatagramSize,
           MaxPayload,
           nbufs),
  _client (src),
  _inlet  (inlet),
  _occPool(sizeof(EvrCommand),64)
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

void EvrServer::dump(int detail) const
  {
  printf("Server %04X (port %d)  fd %d represents EVR\n",
         id(),
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

EvrServer::~EvrServer()
{
}



bool EvrServer::isValued() const
{
  return true;
}

bool EvrServer::isRequired() const
{
  return true;
}

const Xtc&   EvrServer::xtc   () const
{
  return ((Datagram*)_server.datagram())->xtc;
}

const Src&  EvrServer::client  () const
{
  return _client;
}


bool     EvrServer::more  () const
{
  return false;
}

unsigned EvrServer::length() const
{
  return 0;
}

unsigned EvrServer::offset() const
{
  return 0;
}

int EvrServer::pend(int flag)
{
  return _server.pend(flag);
}

#include <stdio.h>

int EvrServer::fetch(char* payload, int flags)
{
  int length = _server.fetch(payload,flags);
  if (length < 0)
    return -1;

  const EvrDatagram& dg = reinterpret_cast<const EvrDatagram&>(*_server.datagram());
  /*
  printf("EvrServer cnt %08x  clock %08x/%08x  pulse %08x\n",
   dg.evr, dg.seq.clock().high, dg.seq.clock().low, dg.seq.highAll(), dg.seq.low());
  */

  const char* cmd = payload;
  for(unsigned i=0; i<dg.ncmds; i++, cmd++) {
    if (_occPool.numberOfFreeObjects())
      _inlet.post(new(&_occPool) EvrCommand(dg.seq,*cmd));
    else
      printf("EvrServer occPool buffer is empty.  EvrCommand dropped!\n");
  }

  return (dg.seq.isEvent()) ? 0 : -1;
}

