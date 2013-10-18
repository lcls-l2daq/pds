#include "EvrFifoServer.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <fcntl.h>

namespace Pds {
  class EvrFifoServerMsg {
  public:
    uint32_t count;
    uint32_t fiducial;
  };
};

using namespace Pds;


EvrFifoServer::EvrFifoServer(const Src& src) :
  _more  (false),
  _xtc   (TypeId(TypeId::Any,0), src)
{
  int err = ::pipe(_fd);
  if (err)
    printf("Error opening EvrFifoServer pipe: %s\n",strerror(errno));
  fd(_fd[0]);
}

EvrFifoServer::~EvrFifoServer()
{
  ::close(_fd[0]);
  ::close(_fd[1]);
}

void EvrFifoServer::post(unsigned count,
                         unsigned fiducial)
{
  EvrFifoServerMsg v;
  v.count=count;
  v.fiducial=fiducial;
  ::write(_fd[1],&v,sizeof(v));
}

void EvrFifoServer::clear()
{
  EvrFifoServerMsg msg;
  int flags = ::fcntl(_fd[0],F_GETFL) | O_NONBLOCK;
  ::fcntl(_fd[0],F_SETFL,flags);
  while(::read(_fd[0],&msg,sizeof(msg))>0) 
    printf("EvrFifoServer::clear %p\n",&msg);
  flags ^= O_NONBLOCK;
  ::fcntl(_fd[0],F_SETFL,flags);
}

void EvrFifoServer::dump(int detail) const
{
}

bool EvrFifoServer::isValued() const
{
  return true;
}

const Src& EvrFifoServer::client() const
{
  return _xtc.src;
}

const Xtc& EvrFifoServer::xtc() const
{
  return _xtc;
}

//
//  Fragment information
//
bool EvrFifoServer::more() const
{
  return false;
}

unsigned EvrFifoServer::length() const
{
  return _xtc.extent;
}

unsigned EvrFifoServer::offset() const
{
  return 0;
}

int EvrFifoServer::pend(int flag)
{
  return 0;
}

unsigned EvrFifoServer::count() const
{
  return _count;
}

int EvrFifoServer::fetch(char* payload, int flags)
{
  EvrFifoServerMsg fmsg;
  int length = ::read(_fd[0],&fmsg,sizeof(fmsg));
  if (length >= 0) {
    _count = fmsg.count;
    Xtc* xtc = new (payload) Xtc(_xtc.contains,_xtc.src);
    *reinterpret_cast<uint32_t*>(xtc->alloc(sizeof(uint32_t))) = fmsg.fiducial;
    return xtc->extent;
  }
  return length;
}
