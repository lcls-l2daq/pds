#ifndef QuadAdc_AdcCore_hh
#define QuadAdc_AdcCore_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {
    class AdcCore {
    public:
      void start_training();
      void loop_checking ();
      void capture_idelay();
      void dump_status   () const;
    private:
      uint32_t _cmd;
      uint32_t _status;
      uint32_t _master_start;
      uint32_t _adrclk_delay_set_auto;
      uint32_t _channel_select;
      uint32_t _tap_match_lo;
      uint32_t _tap_match_hi;
      uint32_t _adc_req_tap ;
      uint32_t _rsvd2[0xf8];
    };
  };
};

#endif
