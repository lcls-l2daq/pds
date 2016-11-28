#ifndef QuadAdc_RingBuffer_hh
#define QuadAdc_RingBuffer_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {

    class RingBuffer {
    public:
      RingBuffer() {}
    public:
      void     enable (bool);
      void     clear  ();
      void     dump   ();
    private:
      uint32_t   _csr;
      uint32_t   _dump[0x1fff];
    };
  };
};

#endif
