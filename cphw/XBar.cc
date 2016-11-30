#include "pds/cphw/XBar.hh"

#include <stdio.h>

using namespace Pds::Cphw;

void XBar::setOut( Map out, Map in )
{
  outMap[unsigned(out)] = unsigned(in);
}

void XBar::dump  () const
{
  for(unsigned i=0; i<4; i++)
    printf("OUT[%u] = %u\n", i, unsigned(outMap[i]));
}
