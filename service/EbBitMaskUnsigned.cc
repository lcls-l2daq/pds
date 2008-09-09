#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "EbBitMaskUnsigned.hh"

using namespace Pds;

int EbBitMaskUnsigned::read(const char* arg, char** end)
{
  errno = 0;
  unsigned long inmask = strtoul(arg, end, 0);
  if (errno) return 1;
  _mask = inmask;
  return 0;
}

int EbBitMaskUnsigned::write(char* buf) const
{
  sprintf(buf, "0x%x", _mask);
  return 0;
}
