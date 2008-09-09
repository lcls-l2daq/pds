#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "EbBitMaskArray.hh"

using namespace Pds;

int EbBitMaskArray::read(const char* arg, char** end)
{
  // Note: implementation of this method must change if BitMaskWords ne 2
  errno = 0;
  unsigned long long inmask = strtoull(arg, end, 0);
  if (errno) return 1;
  _mask[0] = inmask & 0xffffffff;
  inmask >>= 32;
  _mask[1] = inmask & 0xffffffff;
  return 0;
}

int EbBitMaskArray::write(char* buf) const
{
  // Note: implementation of this method must change if BitMaskWords ne 2
  if (_mask[1]) {
    sprintf(buf, "0x%x%08x", _mask[1], _mask[0]);
  } else {
    sprintf(buf, "0x%x", _mask[0]);
  }
  return 0;
}
