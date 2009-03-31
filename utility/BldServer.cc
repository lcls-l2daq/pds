#include "BldServer.hh"

#include "EbEventKey.hh"
#include "pds/xtc/BldDatagram.hh"
#include "Mtu.hh"

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

static const unsigned DatagramSize = sizeof(BldDatagram);
static const unsigned MaxPayload = Mtu::Size;

BldServer::BldServer(const Ins& ins,
		     const Src& src,
		     unsigned   nbufs) :
  _server(-1UL,
	  Port::VectoredServerPort,
	  ins,
	  DatagramSize,
	  MaxPayload,
	  nbufs),
  _client(src),
  _xtc   (TypeId(TypeId::Any,0),src)
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

void BldServer::dump(int detail) const
  {
  printf("Server %04X (port %d) represents client %08X\n",
         id(),
         _server.portId(),
	 _server.address());
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

BldServer::~BldServer()
{
}


bool BldServer::isValued() const
{
  return false;
}

const Xtc&   BldServer::xtc   () const
{
  return _xtc;
}

const Src& BldServer::client () const
{
  return _client;
}

bool     BldServer::more  () const
{
  return false;
}

unsigned BldServer::length() const
{
  return 0;
}

unsigned BldServer::offset() const
{
  return 0;
}

int BldServer::pend(int flag)
{
  return _server.pend(flag);
}

int BldServer::fetch(char* payload, int flags)
{
  Xtc* xtc = new (payload)Xtc(_xtc);
  int length = _server.fetch(xtc->payload(),flags);
  xtc->alloc(length);
  _xtc.alloc(length+sizeof(Xtc)-_xtc.extent);
  /*
  printf("BldServer::fetch  payload %p  flags %x : length %x extent %x/%x\n", 
    	 payload, flags, length, xtc->tag.extent(), _xtc.tag.extent());
  unsigned* p = (unsigned*)xtc;
  unsigned* e = p + ((xtc->sizeofPayload()+sizeof(InXtc))>>2);
  for(int k=0; p<e; k++) {
    printf(" %08x",*p++);
    if (k%8==7) printf("\n");
  }
  printf("\n");
  */
  return length+sizeof(Xtc);
}

int BldServer::fetch(ZcpFragment& zf, int flags)
{
  ZcpFragment zcp;
  int length = _server.fetch(zcp,flags);
  _xtc.alloc(length+sizeof(Xtc)-_xtc.extent);
  zf.uinsert(&_xtc,sizeof(Xtc));
  zf.copy   (zcp, length);
  //  printf("BldServer::fetch  dg %p  flags %x : length %x extent %x\n", 
  //	 &dg, flags, length, _xtc.tag.extent());
  return length;
}
