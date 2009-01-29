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
#include <stropts.h>
#include <stdio.h>


using namespace Pds;

static int _nfd = ::open("/dev/null",O_WRONLY);

//
//  "splice" and "tee" are introduced in gcc version 4
//
#if (__GNUC__ > 3 )
#define SPLICE_FLAGS      (SPLICE_F_MOVE)
#define SPLICE_FLAGS_MORE (SPLICE_F_MOVE | SPLICE_F_MORE)
#else
#define SPLICE_FLAGS      0
#define SPLICE_FLAGS_MORE 0
#endif

static inline int _splice(int fd0, int fd1,
			  int size, int flags)
{
#if ( __GNUC__ > 3 )
//#if defined(__USE_GNU) && defined(SPLICE_F_MOVE)
  return ::splice(fd0, NULL, fd1, NULL, size, flags);
#else
  static bool _warned = false;
  if (!_warned) {
    printf("splice not implemented in GCC_VERSION %d.%d\n",__GNUC__,__GNUC_MINOR__);
    _warned = true;
  }
  return -1;
#endif
}

static inline int _tee(int fd0, int fd1,
		       int size, int flags)
{
#if ( __GNUC__ > 3 )
//#if defined(__USE_GNU) && defined(SPLICE_F_MOVE)
  return ::tee(fd0, fd1, size, flags);
#else
  static bool _warned = false;
  if (!_warned) {
    printf("tee not implemented in GCC_VERSION %d.%d\n",__GNUC__,__GNUC_MINOR__);
    _warned = true;
  }
  return -1;
#endif
}


ZcpFragment::ZcpFragment() :
  _size(0)
{
  int err = pipe(_fd);
  if (err)
    printf("ZcpFragment::ctor error: %s\n", strerror(errno));
}

ZcpFragment::~ZcpFragment()
{
  ::close(_fd[0]);
  ::close(_fd[1]);
}

void ZcpFragment::flush()
{
  if (_size)
    kremove(_size);
}

int ZcpFragment::uinsert(const void* b, int size)
{
  iovec iov;
  iov.iov_base = const_cast<void*>(b);
  iov.iov_len  = size;
  return vinsert(&iov, 1);
}

int ZcpFragment::uremove(void* b, int size)
{
  iovec iov;
  iov.iov_base = b;
  iov.iov_len  = size;
  return vremove(&iov, 1);
}

int ZcpFragment::vinsert(iovec* iov, int n) 
{
  ::fcntl(_fd[1],F_SETFL,O_NONBLOCK);
  int spliced = ::writev(_fd[1], iov, n);
  if (spliced > 0)
    _size += spliced;
  return spliced; 
}

int ZcpFragment::vremove(iovec* iov, int n)
{
  int spliced = ::readv(_fd[0], iov, n);
  if (spliced > 0)
    _size -= spliced;
  return spliced;
}

int ZcpFragment::read(void* buff, int size)
{
  return ::read(_fd[0],buff,size);
}

int ZcpFragment::kinsert(int fd, int size) 
{ 
  int spliced = _splice(fd, _fd[1], size, SPLICE_FLAGS);
  if (spliced > 0) 
    _size += spliced;
  return spliced;
}

int ZcpFragment::copy  (ZcpFragment& dg, int size)
{
  int copied = _tee(dg._fd[0],_fd[1],size, SPLICE_FLAGS);
  if (copied > 0)
    _size += copied;
  return copied;
}

int ZcpFragment::kremove(int size) 
{
  return kremove(_nfd,size);
}

int ZcpFragment::kremove(int fd, int size) 
{
  int flags = size > _size ? SPLICE_FLAGS_MORE : SPLICE_FLAGS;
  int spliced = _splice(_fd[0], fd, size, flags);
  if (spliced > 0)
    _size -= spliced;
  return spliced; 
}

int ZcpFragment::kcopyTo(int fd, int size)
{
  int copied = _tee(_fd[0],fd,size, SPLICE_FLAGS);
  return copied;
}

