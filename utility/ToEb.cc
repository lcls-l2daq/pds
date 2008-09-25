#include "ToEb.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

ToEb::ToEb(const Src& client) :
  _client(client)
{
  int err = ::pipe(_pipefd);
  if (err)
    printf("ToEb::ctor error: %s\n", strerror(errno));
  fd(_pipefd[0]);
}


int ToEb::send(const CDatagram* cdatagram)
{
  const Datagram& datagram = cdatagram->datagram();
  unsigned size = datagram.xtc.sizeofPayload() + sizeof(InXtc);
  /*
  {
    unsigned* d = (unsigned*)&datagram;
    printf("ToEb::send %08x/%08x %08x/%08x %08x\n"
	   "           %08x %08x %08x %08x %08x\n",
	   d[0], d[1], d[2], d[3], d[4],
	   d[5], d[6], d[7], d[8], d[9]);
  }
  */
  ::write(_pipefd[1],&datagram,sizeof(Datagram));
  ::write(_pipefd[1],&datagram.xtc,size);
  return 0;
}

int ToEb::send(const ZcpDatagram* cdatagram)
{
  const Datagram& datagram = cdatagram->datagram();

  ::write(_pipefd[1],&datagram,sizeof(Datagram));

  const LinkedList<ZcpFragment>& fragments = cdatagram->fragments();
  ZcpFragment* zf = fragments.forward();
  while(zf!=fragments.empty()) {
    zf->kremove(_pipefd[1],zf->size());
    zf = zf->forward();
  }
  return 0;
}

/*
int ToEb::send(const InDatagram* indg,
	       const Ins& dst) 
{
  Datagram* datagram  = indg->datagram();
  unsigned size       = datagram->xtc.sizeofPayload() + sizeof(InXtc);

  if (size <= Mtu::Size)  return _client.send(indg, dst);

  IovChunkIterator chkIter(iovArray, iovCount);

  do {
    int error;
    if ((error = _client.send((char*)chkIter.header(), chkIter.iovArray(), chkIter.iovCount(), dst)))
      return error;
  } while (chkIter.next());

  return 0;
}
*/

void ToEb::dump(int detail) const
{
}

bool ToEb::isValued() const
{
  return true;
}

const Src& ToEb::client() const
{
  return _client; 
}

const InXtc& ToEb::xtc() const
{
  return _datagram.xtc;
}

int ToEb::pend(int flag) 
{
  return -1;
}

int ToEb::fetch(char* payload, int flags)
{
  int length = ::read(_pipefd[0],&_datagram,sizeof(Datagram));
  if (length < 0) return length;

  int nbytes = _datagram.xtc.sizeofPayload()+sizeof(InXtc);
  while( nbytes ) {
    length = ::read(_pipefd[0],payload,nbytes);
    if (length < 0) return length;
    nbytes  -= length;
    payload += length;
  }
  /*
  {
    unsigned* d = (unsigned*)&_datagram;
    printf("ToEb::fetch %08x/%08x %08x/%08x %08x\n"
	   "%p %08x %08x %08x %08x %08x\n",
	   d[0], d[1], d[2], d[3], d[4],
	   this, d[5], d[6], d[7], d[8], d[9]);
  }
  */	 
  return _datagram.xtc.sizeofPayload()+sizeof(InXtc);
}

int ToEb::fetch(ZcpFragment& zf, int flags)
{
  int length = ::read(_pipefd[0],&_datagram,sizeof(Datagram));
  if (length < 0) return length;

  int nbytes = _datagram.xtc.sizeofPayload()+sizeof(InXtc);
  return zf.kinsert(_pipefd[0],nbytes);
}

const Sequence& ToEb::sequence() const
{
  return _datagram;
}

unsigned ToEb::count() const
{
  return *(unsigned*)&_datagram;
}
