#ifndef Xpm_Reg64_hh
#define Xpm_Reg64_hh

#include "Reg.hh"

namespace Xpm {
  class Reg64 {
  public:
    Reg64& operator=(const uint32_t v) { low=unsigned(v); return *this; }
    operator uint64_t() const { uint64_t v=unsigned(high); v<<=32; v|=unsigned(low); return v; }
    operator uint32_t() const { return unsigned(low); }
  public:
    Reg low;
    Reg high;
  };
};

#endif
