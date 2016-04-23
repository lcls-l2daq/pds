#ifndef Xpm_Reg_hh
#define Xpm_Reg_hh

#include <stdint.h>

namespace Xpm {
  class Reg {
  public:
    Reg& operator=(const unsigned);
    operator unsigned() const;
  public:
    void setBit  (unsigned);
    void clearBit(unsigned);
  public:
    static void set(unsigned ip, unsigned short port, unsigned mem);
  private:
    uint32_t _reserved;
  };
};

#endif
