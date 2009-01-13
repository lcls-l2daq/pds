#include "ToEb.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;


static const int Post_CDatagram  =1;
static const int Post_CFragment  =2;
static const int Post_ZcpDatagram=3;
static const int Post_ZcpFragment=4;

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
  int msg = Post_CDatagram;
  ::write(_pipefd[1],&msg,sizeof(msg));
  ::write(_pipefd[1],&cdatagram,sizeof(CDatagram*));
  return 0;
}

int ToEb::send(const ZcpDatagram* cdatagram)
{
  int msg = Post_ZcpDatagram;
  ::write(_pipefd[1],&msg,sizeof(msg));
  ::write(_pipefd[1],&cdatagram,sizeof(ZcpDatagram*));
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
  else if (msg == Post_ZcpDatagram) {
    ZcpDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    char* p = payload;
    int remaining = _datagram.xtc.sizeofPayload();
    while( remaining ) {
      int siz = dg->_stream.remove(_fragment,remaining);
      _fragment.uremove(p,siz);
      p         += siz;
      remaining -= siz;
    }
    delete dg;
    length = p - payload;
  }

  return length;
}

int ToEb::fetch(ZcpFragment& zfo, int flags)
{
  _more   = false;

  int msg;
  int length = ::read(_pipefd[0],&msg,sizeof(msg));
  if (length < 0) goto read_err;

  if (msg == Post_CDatagram) {
    CDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    int sz = dg->datagram().xtc.sizeofPayload();
    if ((length = zfo.uinsert( dg->datagram().xtc.payload(), sz )) < 0)
      goto read_err;
    
    if (length != sz) {  // fragmentation
      _more   = true;
      _offset = 0;
      _next   = length;
      msg     = Post_CFragment;
      iovec iov[2];
      iov[0].iov_base = &msg; iov[0].iov_len = sizeof(msg);
      iov[1].iov_base = &dg ; iov[1].iov_len = sizeof(CDatagram*);
      ::writev(_pipefd[1],iov,2);
      //      ::write(_pipefd[1],&msg,sizeof(msg));
      //      ::write(_pipefd[1],&dg,sizeof(CDatagram*));
    }
    else
      delete dg;
  }
  else if (msg == Post_CFragment) {
    _more = true;
    CDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    int remaining = _datagram.xtc.sizeofPayload()-_next;
    if ((length = zfo.uinsert( dg->datagram().xtc.payload()+_next, remaining )) < 0)
      goto read_err;

    _offset = _next;
    _next  += length;
    if (length != remaining) {
      iovec iov[2];
      iov[0].iov_base = &msg; iov[0].iov_len = sizeof(msg);
      iov[1].iov_base = &dg ; iov[1].iov_len = sizeof(CDatagram*);
      ::writev(_pipefd[1],iov,2);
      //      ::write(_pipefd[1],&msg,sizeof(msg));
      //      ::write(_pipefd[1],&dg,sizeof(CDatagram*));
    }
    else
      delete dg;
  }
  else if (msg == Post_ZcpDatagram) {
    ZcpDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    int sz = _datagram.xtc.sizeofPayload();
    if ((length = dg->_stream.remove(zfo,sz)) < 0) 
      goto read_err;

    if (length != sz) {  // fragmentation
      _more   = true;
      _offset = 0;
      _next   = length;
      msg = Post_ZcpFragment;
      ::write(_pipefd[1],&msg,sizeof(msg));
      ::write(_pipefd[1],&dg,sizeof(ZcpDatagram*));
    }      
    else
      delete dg;
  } else if (msg == Post_ZcpFragment) {
    _more = true;
    ZcpDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    int remaining = _datagram.xtc.sizeofPayload()-_next;
    if ((length  = dg->_stream.remove(zfo, remaining)) < 0)
      goto read_err;

    _offset = _next;
    _next  += length;
    if (length != remaining) {
      ::write(_pipefd[1],&msg,sizeof(msg));
      ::write(_pipefd[1],&dg,sizeof(ZcpDatagram*));
    }
    else
      delete dg;
  }

  return length;

 read_err:
  printf("ToEb::fetchz msg %d %s\n",msg,strerror(errno));
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
