#include "AcqServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "DmaEngine.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

AcqServer::AcqServer(const Src& client) :
  _client(client)
{
  int err = ::pipe(_pipefd);
  if (err)
    printf("AcqServer::ctor error: %s\n", strerror(errno));
  fd(_pipefd[0]);
}

void AcqServer::setDma(DmaEngine* dma) {
  _dma=dma;
}

int AcqServer::payloadComplete()
{
  const Command cmd = Payload;
  ::write(_pipefd[1],&cmd,sizeof(cmd));
  return 0;
}

int AcqServer::headerComplete(Datagram& dg)
{
  const Command cmd = Header;
  ::write(_pipefd[1],&cmd,sizeof(cmd));
  ::write(_pipefd[1],&dg,sizeof(Datagram));
  return 0;
}

unsigned AcqServer::offset() const {
  if (_cmd==Header) {
    return 0;
  } else {
    return sizeof(InXtc);
  }
}

void AcqServer::dump(int detail) const
{
}

bool AcqServer::isValued() const
{
  return true;
}

const Src& AcqServer::client() const
{
  return _client; 
}

const InXtc& AcqServer::xtc() const
{
  return _datagram.xtc;
}

int AcqServer::pend(int flag) 
{
  return -1;
}

unsigned AcqServer::length() const {
  return _datagram.xtc.extent;
}

int AcqServer::fetch(char* payload, int flags)
{
  int length = ::read(_pipefd[0],&_cmd,sizeof(_cmd));
  if (length < 0) return length;

  if (_cmd == Header) {
    int nbytes = sizeof(Datagram);
    while( nbytes ) {
      length = ::read(_pipefd[0],&_datagram,nbytes);
      memcpy(payload,&_datagram.xtc,sizeof(_datagram.xtc));
      if (length < 0) return length;
      nbytes  -= length;
      payload += length;
    }
    _dma->start(payload+sizeof(InXtc));
    return sizeof(InXtc);
  } else if (_cmd == Payload) {
    return xtc().sizeofPayload();
  } else {
    printf("Unknown command in Acqserver 0x%x\n",_cmd);
    return -1;
  }
}

int AcqServer::fetch(ZcpFragment& zf, int flags)
{
  return 0;
}

const Sequence& AcqServer::sequence() const
{
  return _datagram.seq;
}

unsigned AcqServer::count() const
{
  return *(unsigned*)&_datagram;
}

bool AcqServer::more() const { return true; }
