#include "TStream.hh"

#include "ZcpFragment.hh"

#include <sys/types.h>
#include <sys/socket.h>

#if ( __GNUC__ > 3 )
#include "pds/zerocopy/tub/tub.h"
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>


using namespace Pds;

TStream::TStream()
{
#if ( __GNUC__ > 3 )
  _fd[0] = tub_open();
  if (_fd[0] < 0)
    printf("TStream::TStream error opening stream : %s\n",
	   strerror(errno));
#else
  printf("TStream::TStream GCC_VERSION %d.%d\n",__GNUC__,__GNUC_MINOR__);
#endif
}

TStream::~TStream()
{
#if ( __GNUC__ > 3 )
  tub_close(_fd[0]);
#endif
}

int TStream::insert(ZcpFragment& zf, int size)
{
  int ret = 0;
  int len;
  do {
    len = zf.kremove(_fd[0],size);
    size -= len;
    ret  += len;
  } while (len && size);
  return ret;
}

int TStream::remove(ZcpFragment& zf, int size)
{
  int ret = 0;
  int len;
  do {
    len = zf.kinsert(_fd[0],size);
    size -= len;
    ret  += len;
  } while (len && size);
  return ret;
}

