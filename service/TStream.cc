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
  _fd = tub_open();
  if (_fd < 0)
    printf("TStream::TStream error opening stream : %s\n",
	   strerror(errno));
#else
  printf("TStream::TStream GCC_VERSION %d.%d\n",__GNUC__,__GNUC_MINOR__);
#endif
}

TStream::~TStream()
{
#if ( __GNUC__ > 3 )
  tub_close(_fd);
#endif
}

int TStream::insert(ZcpFragment& zf, int size)
{
  return zf.kremove(_fd,size);
}

int TStream::remove(ZcpFragment& zf, int size)
{
  return zf.kinsert(_fd,size);
}

int TStream::flush()
{
#if ( __GNUC__ > 3 )
  return tub_flush(_fd);
#else
  return -1;
#endif
}

int TStream::dump() const
{
#if ( __GNUC__ > 3 )
  return tub_debug(_fd);
#else
  return -1;
#endif
}
