#include "KStream.hh"

#include "ZcpFragment.hh"

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include "pds/zerocopy/kmemory/kmemory.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>


using namespace Pds;

KStream::KStream()
{
  _fd[0] = kmemory_open();
  if (_fd[0] < 0)
    printf("KStream::KStream error opening stream : %s\n",
	   strerror(errno));
}

KStream::~KStream()
{
  kmemory_close(_fd[0]);
}

int KStream::insert(ZcpFragment& zf, int size)
{
  return zf.kremove(_fd[0],size);
}

int KStream::remove(int size)
{
  return kmemory_skip(_fd[0], size, KMEMORY_READ);
}

int KStream::remove(ZcpFragment& zf, int size)
{
  int len = kmemory_set_passthru(_fd[0],size);
  if (len != size)
    printf("kmemory_set_passthru %d/%d bytes\n",len,size);
  return zf.kinsert(_fd[0],len);
}

int KStream::map(iovec* iov, int niov)
{
  return kmemory_map_read(_fd[0],iov,niov);
}

int KStream::unmap(iovec* iov, int niov)
{
  return kmemory_unmap(_fd[0],iov,niov);
}
