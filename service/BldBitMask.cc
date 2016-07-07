#include "pds/service/BldBitMask.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

namespace Pds {
  const static unsigned HEADER = 2;
  const static unsigned NCHARS = 16;

  template<unsigned N>
  void BitMaskArray<N>::print() const
  {
    for (unsigned j = N; j > 0; j--)
      printf("%08x", value(j-1));
  }

  template<unsigned N>
  int BitMaskArray<N>::read(const char* arg, char** end)
  {
    if (strlen(arg) > HEADER && (strncmp("0x", arg, HEADER) == 0 || strncmp("0X", arg, HEADER) == 0)) {
      char buf[NCHARS+1];
      const char *data = &arg[HEADER];
      int end_index = strlen(data);
      int beg_index = end_index - NCHARS;
      if (beg_index < 0) beg_index = 0;
      unsigned cur_mask = 0;
      do {
        if (cur_mask >= (N-1))
          return 1;
        strncpy(&buf[0], &data[beg_index], (size_t) end_index - beg_index);
        buf[end_index - beg_index] = 0;
        errno = 0;
        unsigned long long inmask = strtoull(&buf[0], end, NCHARS);
        if (errno) return 1;
        _mask[cur_mask] = inmask & 0xffffffff;
        inmask >>= NCHARS*2;
        cur_mask++;
        _mask[cur_mask] = inmask & 0xffffffff;
        cur_mask++;
        beg_index -= NCHARS;
        end_index -= NCHARS;
        if (beg_index < 0) beg_index = 0;
        if (end_index < 0) end_index = 0;
      } while(end_index > beg_index);
      return 0;
    } else {
      // TODO read in of octal and decimal values
      return 1;
    }
  }

  template <unsigned N>
  int BitMaskArray<N>::write(char* buf) const
  {
    for (unsigned j = N; j > 0; j--) {
      if (j==N) sprintf(buf, "0x%x", value(j-1));
      else sprintf(buf, "%08x", value(j-1));
    }
    return 0;
  }

  // Explicitly declare the ones needed for the standard BLD size (256 bits)
  template void BitMaskArray<PDS_BLD_MASKSIZE>::print() const;
  template int  BitMaskArray<PDS_BLD_MASKSIZE>::read(const char* arg, char** end);
  template int  BitMaskArray<PDS_BLD_MASKSIZE>::write(char* buf) const;
}
