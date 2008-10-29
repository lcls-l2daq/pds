#include "AcqServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/utility/DmaEngine.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

AcqServer::AcqServer(const Src& client) :
  _xtc(TypeNum::Id_Waveform,client)
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

int AcqServer::headerComplete(unsigned payloadSize,
			      unsigned count)
{
  Command cmd = Header;
  iovec iov[3];
  iov[0].iov_base = &cmd; 
  iov[0].iov_len = sizeof(cmd);
  iov[1].iov_base = &payloadSize; 
  iov[1].iov_len = sizeof(payloadSize);
  iov[2].iov_base = &count;
  iov[2].iov_len = sizeof(count);

  ::writev(_pipefd[1], iov, 3);
  return 0;
}

unsigned AcqServer::offset() const {
  if (_cmd==Header) {
    return 0;
  } else {
    return sizeof(Xtc);
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
  return _xtc.src; 
}

const Xtc& AcqServer::xtc() const
{
  return _xtc;
}

int AcqServer::pend(int flag) 
{
  return -1;
}

unsigned AcqServer::length() const {
  return _xtc.extent;
}

int AcqServer::fetch(char* payload, int flags)
{
  int length = ::read(_pipefd[0],&_cmd,sizeof(_cmd));
  if (length < 0) return length;

  if (_cmd == Header) {
    unsigned payloadSize;
    length = ::read(_pipefd[0],&payloadSize,sizeof(payloadSize));
    if (length < 0) return length;
    length = ::read(_pipefd[0],&_count,sizeof(_count));
    if (length < 0) return length;
    _xtc.extent = payloadSize+sizeof(Xtc);
    memcpy(payload, &_xtc, sizeof(Xtc));
    _dma->start(payload+sizeof(Xtc));
    return sizeof(Xtc);
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

unsigned AcqServer::count() const
{
  return _count;
}

bool AcqServer::more() const { return true; }
