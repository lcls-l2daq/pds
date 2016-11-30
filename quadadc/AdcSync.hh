#ifndef QuadAdc_AdcSync_hh
#define QuadAdc_AdcSync_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {
    class AdcSync {
    public:
      void set_delay     (const uint32_t*);
      void start_training();
      void stop_training ();
      void dump_status   () const;
    private:
      uint32_t _cmd;
      mutable uint32_t _select;
      uint32_t _stats;
      uint32_t _rsvd;
      uint32_t _delay[8];
      uint32_t _rsvd2[0xf4];
    };
  };
};

#endif
