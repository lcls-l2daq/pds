#ifndef QuadAdc_FmcSpi_hh
#define QuadAdc_FmcSpi_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {
    class FmcSpi {
    public:
      int initSPI();
      int resetSPIclocktree();
      int resetSPIadc      ();

      enum SyncSrc { NoSync=0, FPGA=1 };
      int cpld_init();
        
      int clocktree_init   (unsigned clksrc, 
                            unsigned vcotype);
      int adc_enable_test  (unsigned pattern);
      int adc_disable_test ();
    private:
      void     _writeAD9517(unsigned addr, unsigned value);
      void     _writeADC   (unsigned addr, unsigned value);
      void     _writeCPLD  (unsigned addr, unsigned value);
      unsigned _readAD9517 (unsigned addr);
      unsigned _readADC    (unsigned addr);
      unsigned _readCPLD   (unsigned addr);
      void     _applySync();
    private:
      uint32_t _reg[256]; // Bridge configuration access
      uint32_t _sp1[256]; // SPI device access 1B address
      uint32_t _sp2[256]; // SPI device access 2B address
    };
  };
};

#endif
