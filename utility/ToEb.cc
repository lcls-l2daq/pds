#include "ToEb.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;


static const int Post_CDatagram  =1;
static const int Post_ZcpDatagram=2;
static const int Post_ZcpFragment=3;

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
  _more = false;

  int msg;
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

    length += sizeof(InXtc);
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    memcpy(payload,
	   &dg->datagram().xtc,
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
    memcpy(p, &dg->datagram().xtc, sizeof(InXtc));
    p      += sizeof(InXtc);
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
  if (msg == Post_CDatagram) {
    CDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    length = dg->datagram().xtc.sizeofPayload();
    length += sizeof(InXtc);
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    int nbytes = zfo.uinsert( const_cast<InXtc*>(&dg->datagram().xtc), length );
    if (length != nbytes) {
      printf("ToEb::fetch ZcpFragment payload truncated %x/%x\n",
	     nbytes, length);
      handleError(ENOMEM);
      length = -1;
    }
    delete dg;
  }
  else if (msg == Post_ZcpDatagram) {
    ZcpDatagram* dg;
    ::read(_pipefd[0],&dg,sizeof(dg));
    memcpy(&_datagram,
	   &dg->datagram(),
	   sizeof(Datagram));
    length = sizeof(InXtc);
    zfo.uinsert(const_cast<InXtc*>(&dg->datagram().xtc), length);
    length += dg->_stream.remove(zfo, _datagram.xtc.sizeofPayload());
    if (length != _datagram.xtc.extent) {
      _more   = true;
      _offset = 0;
      _next   = length;
      printf("ToEb::chunk %d/%d\n",_next,_datagram.xtc.extent);
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
    int remaining = _datagram.xtc.extent-_next;
    length  = dg->_stream.remove(zfo, remaining);
    _offset = _next;
    _next  += length;
    printf("ToEb::chunk %d/%d\n",_next,_datagram.xtc.extent);
    if (length != remaining) {
      ::write(_pipefd[1],&msg,sizeof(msg));
      ::write(_pipefd[1],&dg,sizeof(ZcpDatagram*));
    }
    else
      delete dg;
  }

  return length;
}

const Sequence& ToEb::sequence() const
{
  return _datagram.seq;
}

unsigned ToEb::count() const
{
  return *(unsigned*)&_datagram;
}
