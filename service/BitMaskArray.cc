#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "pds/service/BitMaskArray.hh"

template <unsigned N>
void Pds::BitMaskArray<N>::print() const
{
  for (unsigned j = N; j > 0; j--)
    printf("%08x", value(j-1));
}
