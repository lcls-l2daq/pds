#include "ToEb.hh"
#include "pds/xtc/CDatagram.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds;


static const int Post_CDatagram  =1;
static const int Post_CFragment  =2;

ToEb::ToEb(const Src& client) :
  _client(client),
  _datagram(TypeId(TypeId::Any,0),client)
{
  int err = ::pipe(_pipefd);
  if (err)
    printf("ToEb::ctor error: %s\n", strerror(errno));
  fd(_pipefd[0]);
}


int ToEb::send(const CDatagram* cdatagram)
{
  int msg = Post_CDatagram;
  ::write(_pipefd[1],&msg,sizeof(msg));
  ::write(_pipefd[1],&cdatagram,sizeof(CDatagram*));
  return 0;
}

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

const Xtc& ToEb::xtc() const
{
  return _datagram.xtc;
}

int ToEb::pend(int flag) 
{
  return -1;
}

int ToEb::fetch(char* payload, int flags)
{
  _more = false;

  int msg = -1;
  int length = ::read(_pipefd[0],&msg,sizeof(msg));
  if (length==-1) {
    handleError(errno);
    return length;
  }
  if (msg == Post_CDatagram) {
    CDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    length  = dg->datagram().xtc.sizeofPayload();

    if (length < 0) {
      printf("ToEb::fetch received cdg %p  payload length %d\n",dg,length);
    }

    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    memcpy(payload,
 	   dg->datagram().xtc.payload(),
 	   length);
    delete dg;
  }

  return length;
}

const Sequence& ToEb::sequence() const
{
  return _datagram.seq;
}

const Env& ToEb::env() const
{
  return _datagram.env;
}
