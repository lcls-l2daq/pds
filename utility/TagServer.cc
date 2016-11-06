#include "pds/utility/TagServer.hh"
#include "pds/utility/Inlet.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/Mtu.hh"
#include "pds/xtc/EvrDatagram.hh"

using namespace Pds;

static const unsigned DatagramSize = sizeof(EvrDatagram);
static const unsigned MaxPayload = Mtu::Size;

TagServer::TagServer(const Ins& ins,
                     const Src& src,
                     Inlet&     inlet,
                     unsigned   nbufs ) :
  _server    ((unsigned)-1,
              ins,
              DatagramSize,
              MaxPayload,
              nbufs ),
  _client    (src),
  _inlet     (inlet),
  _occPool   (sizeof(EvrCommand),64)
{
  _payload = new char[MaxPayload];
  fd(_server.fd());
}

TagServer::~TagServer() { delete[] _payload; }

const Sequence& TagServer::sequence() const
{
  return reinterpret_cast<const EvrDatagram*>(_server.datagram())->seq;
}

const Env&      TagServer::env     () const
{
  return reinterpret_cast<const EvrDatagram*>(_server.datagram())->env;
}

void TagServer::dump(int detail) const
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

bool TagServer::isValued() const
{
  return true;
}

bool TagServer::isRequired() const
{
  return true;
}

//  This should never be called
const Xtc&   TagServer::xtc   () const
{
  return *(Xtc*)0;
}

const Src& TagServer::client () const
{
  return _client;
}

bool     TagServer::more  () const
{
  return false;
}

unsigned TagServer::length() const
{
  return 0;
}

unsigned TagServer::offset() const
{
  return 0;
}

int TagServer::pend(int flag)
{
  return _server.pend(flag);
}

int TagServer::fetch(char* payload, int flags)
{
  int length = _server.fetch(_payload,flags);
  if (length < 0)
    return -1;

  const EvrDatagram& dg = reinterpret_cast<const EvrDatagram&>(*_server.datagram());

  const char* cmd = _payload;
  for(unsigned i=0; i<dg.ncmds; i++, cmd++) {
    if (_occPool.numberOfFreeObjects())
      _inlet.post(new(&_occPool) EvrCommand(dg.seq,*cmd));
    else
      printf("TagServer occPool buffer is empty.  EvrCommand dropped!\n");
  }

  return (dg.seq.isEvent()) ? 0 : -1;
}
