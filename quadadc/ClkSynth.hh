#ifndef QuadAdc_ClkSynth_hh
#define QuadAdc_ClkSynth_hh

#include <stdint.h>
#include <stdio.h>

namespace Pds {
  namespace QuadAdc {
    class ClkSynth {
    public:
      void dump () const;
      void setup();
    public:
      volatile uint32_t _reg[256];
    };
  };
};

#endif
