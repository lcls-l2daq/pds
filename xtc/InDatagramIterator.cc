#include "InDatagramIterator.hh"

#include <string.h>

using namespace Pds;

int InDatagramIterator::copy( void* vdst,
			      int size )
{
  unsigned char* dst = (unsigned char*)vdst;
  iovec iov[1];
  int remaining = size;
  while( remaining ) {
    int sz = read(iov,1,remaining);
    if (!sz) break;
    memcpy(dst,iov[0].iov_base,sz);
    dst       += sz;
    remaining -= sz;
  } 
  return size;
}

