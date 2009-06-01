#include "pds/service/BitMaskArray.cc"

namespace Pds {

  template class BitMaskArray<2>;
  template class BitMaskArray<4>;

  template<>
  int Pds::BitMaskArray<2>::read(const char* arg, char** end)
  {
    errno = 0;
    unsigned long long inmask = strtoull(arg, end, 0);
    if (errno) return 1;
    _mask[0] = inmask & 0xffffffff;
    inmask >>= 32;
    _mask[1] = inmask & 0xffffffff;
    return 0;
  }

  template<>
  int Pds::BitMaskArray<2>::write(char* buf) const
  {
    // Note: implementation of this method must change if BitMaskWords ne 2
    if (_mask[1]) {
      sprintf(buf, "0x%x%08x", _mask[1], _mask[0]);
    } else {
      sprintf(buf, "0x%x", _mask[0]);
    }
    return 0;
  }

}
