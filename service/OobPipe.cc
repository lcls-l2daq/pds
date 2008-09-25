#include "OobPipe.hh"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds;

OobPipe::OobPipe(int sizeofDatagram) :
  _sizeofDatagram(sizeofDatagram)
{
  int err = ::pipe(_pfd);
  if (err) {
  }

  fd(_pfd[0]);
}

OobPipe::~OobPipe()
{
  ::close(_pfd[0]);
  ::close(_pfd[1]);
}

int OobPipe::pend(int flags)
{
  char* body = payload();

  int length = ::read(_pfd[0], _datagram, _sizeofDatagram);
  if (!length) return handleError(errno);
  
  int nbytes = ::read(_pfd[0], body, -1UL);

  int repend;

  if(nbytes != - 1)
    {
      length = _sizeofDatagram + nbytes;
      if(length < 0) length = 0;
      repend = commit(_datagram, body, length);
    }
  else
    repend = handleError(errno);

  return repend;
}

int      OobPipe::fetch       (char* payload, int flags)
{
  int length = ::read(_pfd[0], _datagram, _sizeofDatagram);
  if (!length) return handleError(errno);
  
  int nbytes = ::read(_pfd[0], payload, -1UL);

  if(nbytes >= 0)
    {
    length -= _sizeofDatagram;
    length += nbytes;
    if(length < 0) length = 0;
    }
  else
    {
      printf("OobPipe::fetch failed payload %p  flags %x  socket %d\n",
	     payload, flags, _pfd[0]);
    handleError(errno);
    }

  return length;
}

int      OobPipe::fetch       (ZcpFragment& dg, int flags)
{
  int length = dg.kinsert(_pfd[0], -1);

  if (length == -1) {
    printf("OobPipe::fetch failed ZcpFragment %p  flags %x  socket %d\n",
	   &dg, flags, _pfd[0]);
    handleError(errno);
  }

  return length-_sizeofDatagram;
}

int OobPipe::unblock(char* dg)
{
  int length = ::write(_pfd[1],dg,_sizeofDatagram);
  return length;
}

int OobPipe::unblock(char* dg, char* payload, int size)
{
  int length = ::write(_pfd[1],dg,_sizeofDatagram);
  length += ::write(_pfd[1],payload,size);
  return length;
}

int OobPipe::unblock(char* dg, char* payload, int size, LinkedList<ZcpFragment>& frags)
{
  int length = ::write(_pfd[1],dg,_sizeofDatagram);
  length += ::write(_pfd[1],payload,size);

  ZcpFragment* frag = frags.forward();
  while(frag != frags.empty()) {
    length += frag->kremove(_pfd[1], frag->size());
    frag = frag->forward();
  }
  return length;
}

char*    OobPipe::payload() { return 0; }

int      OobPipe::commit(char* datagram,
			 char* payload,
			 int payloadSize) { return 1; }

int      OobPipe::handleError(int value) { return 0; }
