#ifndef QuadAdc_Adt7411_hh
#define QuadAdc_Adt7411_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {
    class Adt7411 {
    public:
      unsigned deviceId       () const;
      unsigned manufacturerId () const;
      unsigned interruptStatus() const;
      unsigned interruptMask  () const;
      unsigned internalTemp   () const;
      unsigned externalTemp   () const;
    public:
      void     interruptMask  (unsigned);
    private:
      uint32_t _reg[256];
    };
  };
};

#endif
