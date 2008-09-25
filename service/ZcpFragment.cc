//
//  I still have to consider the fact that pipes have a limited number of
//  pipe buffers (16).  That means that a splice could block (O_NONBLOCK=0) or
//  return -EAGAIN.  The user should learn of the failure and be able to deal 
//  with it (by draining before continuing fill).
//
//
#include "ZcpFragment.hh"

#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds;

ZcpFragment::ZcpFragment() : _size(0)
{
  int err = pipe(_fd);
  if (err)
    printf("ZcpFragment::ctor error: %s\n", strerror(errno));
}

ZcpFragment::ZcpFragment(const ZcpFragment& zfrag) :
  _size(0)
{
  copyFrom(const_cast<ZcpFragment&>(zfrag), zfrag._size);
}

ZcpFragment::~ZcpFragment()
{
  close(_fd[0]);
  close(_fd[1]);
}

//
//  "splice" is introduced in gcc version 4
//
//#if ( __GNUC__ > 3 )
#if 1

int ZcpFragment::kinsert(int fd, int size) 
{ 
  int spliced = ::splice(fd, NULL, _fd[1], NULL, size, SPLICE_F_NONBLOCK);
  _size += spliced;
  return spliced;
}

int ZcpFragment::uinsert(void* b, int size)
{
  iovec iov;
  iov.iov_base = b;
  iov.iov_len  = size;
  return vinsert(&iov, 1);
}

int ZcpFragment::vinsert(iovec* iov, int n) 
{
  int spliced = ::vmsplice(_fd[1], iov, n, SPLICE_F_NONBLOCK);
  if (spliced > 0)
    _size += spliced;
  return spliced; 
}

int ZcpFragment::insert(ZcpFragment& dg, int size) 
{ 
  int spliced = ::splice(dg._fd[0], NULL, _fd[1], NULL, size, SPLICE_F_NONBLOCK);
  _size      += spliced;
  dg._size   -= spliced;
  return spliced;
}

int ZcpFragment::copy  (ZcpFragment& dg, int size)
{
  int copied = ::tee(dg._fd[0],_fd[1],size, SPLICE_F_NONBLOCK);
  if (copied > 0)
    _size += copied;
  return copied;
}

int ZcpFragment::kremove(int size) 
{
  int spliced = ::lseek(_fd[0], size, SEEK_CUR);
  if (spliced < 0)
    printf("ZcpFragment::kremove failed %s\n",strerror(errno));
  return spliced;
}

int ZcpFragment::kremove(int fd, int size) 
{
  int spliced = ::splice(_fd[0], NULL, fd, NULL, size, SPLICE_F_NONBLOCK);
  if (spliced > 0)
    _size -= spliced;
  return spliced; 
}

int ZcpFragment::uremove(void* b, int size)
{
  iovec iov;
  iov.iov_base = b;
  iov.iov_len  = size;
  return vremove(&iov, 1);
}

int ZcpFragment::vremove(iovec* iov, int n)
{
  int spliced = ::vmsplice(_fd[0], iov, n, SPLICE_F_NONBLOCK);
  if (spliced > 0)
    _size -= spliced;
  return spliced;
}

int ZcpFragment::read(void* buff, int size)
{
  return ::read(_fd[0],buff,size);
}

int ZcpFragment::kcopyTo(int fd, int size)
{
  int copied = ::tee(_fd[0],fd,size, SPLICE_F_NONBLOCK);
  return copied;
}

#else

#include <stdio.h>

static inline int _showGccVersion()
{
  printf("GCC_VERSION %d.%d\n",__GNUC__,__GNUC_MINOR__);
  return 0;
}

int ZcpFragment::kinsert(int fd, int size) { return _showGccVersion(); }

int ZcpFragment::uinsert(void* b, int size)
{
  iovec iov;
  iov.iov_base = b;
  iov.iov_len  = size;
  return vinsert(&iov, 1);
}

int ZcpFragment::vinsert(iovec* iov, int n) { return _showGccVersion(); }
int ZcpFragment::insert(ZcpFragment& dg, int size) { return _showGccVersion(); }
int ZcpFragment::kremove(int size) { return _showGccVersion(); }

int ZcpFragment::kremove(int fd, int size) { return _showGccVersion(); }
int ZcpFragment::uremove(void* b, int size)
{
  iovec iov;
  iov.iov_base = b;
  iov.iov_len  = size;
  return vremove(&iov, 1);
}

int ZcpFragment::vremove(iovec* iov, int n) { return _showGccVersion(); }

int ZcpFragment::read(void* buff, int size)
{
  return ::read(_fd[0],buff,size);
}

int ZcpFragment::kcopyTo(int fd, int size) { return _showGccVersion(); }

#endif
