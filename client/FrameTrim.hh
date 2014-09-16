#ifndef Pds_FrameTrim_hh
#define Pds_FrameTrim_hh

#include <stdint.h>
#include <stdlib.h>

namespace Pds {
  class Src;
  class Xtc;

  class FrameTrim {
  public:
    FrameTrim(uint32_t*& pwrite, const Src& src);
  private:
    void _write(const void* p, ssize_t sz);
  public:
    void iterate(Xtc* root);
    void process(Xtc* xtc);
  private:
    uint32_t*& _pwrite;
    const Src& _src;
  };
};

#endif
