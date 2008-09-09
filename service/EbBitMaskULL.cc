#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "EbBitMaskULL.hh"

using namespace Pds;

int EbBitMaskULL::read(const char* arg, char** end)
{
  errno = 0;
  unsigned long long inmask = strtoull(arg, end, 0);
  if (errno) return 1;
  _mask = inmask;
  return 0;
}

int EbBitMaskULL::write(char* buf) const
{
  sprintf(buf, "0x%llx", _mask);
  return 0;
}
