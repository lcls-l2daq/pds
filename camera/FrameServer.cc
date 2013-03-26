#include "FrameServer.hh"

#include "pds/camera/FrameHandle.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/xtc/XtcType.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <fcntl.h>

using namespace Pds;

FrameServer::FrameServer(const Src& src) :
  _more  (false),
  _xtc   (_xtcType, src)
{
  int err = ::pipe(_fd);
  if (err)
    printf("Error opening FexFrameServer pipe: %s\n",strerror(errno));
  fd(_fd[0]);
}

FrameServer::~FrameServer()
{
  ::close(_fd[0]);
  ::close(_fd[1]);
}

void FrameServer::post(FrameServerMsg* msg)
{
  ::write(_fd[1],&msg,sizeof(msg));
}

void FrameServer::clear()
{
  FrameServerMsg* msg;
#if 0
  //  Possible deadlock ?
  while(::read(_fd[0],&msg,sizeof(msg))) ;
#else
  int flags = ::fcntl(_fd[0],F_GETFL) | O_NONBLOCK;
  ::fcntl(_fd[0],F_SETFL,flags);
  while(::read(_fd[0],&msg,sizeof(msg))>0) 
    printf("FrameServer::clear %p\n",msg);
  flags ^= O_NONBLOCK;
  ::fcntl(_fd[0],F_SETFL,flags);
#endif
}

void FrameServer::dump(int detail) const
{
}

bool FrameServer::isValued() const
{
  return true;
}

const Src& FrameServer::client() const
{
  return _xtc.src;
}

const Xtc& FrameServer::xtc() const
{
  return _xtc;
}

//
//  Fragment information
//
bool FrameServer::more() const
{
  return _more;
}

unsigned FrameServer::length() const
{
  return _xtc.extent;
}

unsigned FrameServer::offset() const
{
  return _offset;
}

int FrameServer::pend(int flag)
{
  return 0;
}

unsigned FrameServer::count() const
{
  return _count;
}

