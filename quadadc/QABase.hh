#ifndef QuadAdc_QABase_hh
#define QuadAdc_QABase_hh

#include <stdint.h>

namespace Pds {
  namespace QuadAdc {

    class QABase {
    public:
      void init();
      void setChannels(unsigned);
      enum Interleave { Q_NONE, Q_ABCD };
      void setMode    (Interleave);
      void setupDaq (unsigned partition,
                     unsigned length);
      void setupLCLS(unsigned rate,
                     unsigned length);
      void setupLCLSII(unsigned rate,
                     unsigned length);
      void enableDmaTest(bool);
      void start();
      void stop ();
      void resetCounts();
      void resetClock(bool);
      bool clockLocked() const;
      void dump() const;
    public:
      uint32_t irqEnable;
      uint32_t irqStatus;
      uint32_t partitionAddr;
      uint32_t dmaFullThr;
      uint32_t csr; 
      //   CSR
      // [ 0:0]  count reset
      // [ 1:1]  dma size histogram enable
      // [ 2:2]  dma test pattern enable
      // [ 3:3]  adc sync reset
      // [31:31] acqEnable
      uint32_t acqSelect;
      //   AcqSelect
      // [12: 0]  rateSel  :   [7:0] eventCode
      // [31:13]  destSel
      uint32_t control;
      //   Control
      // [3:0] channel enable mask
      // [8:8] interleave
      // [19:16] partition
      uint32_t samples;       //  Must be a multiple of 16
      uint32_t prescale;
      uint32_t offset;        //  Not implemented
      uint32_t countAcquire;
      uint32_t countEnable;
      uint32_t countInhibit;
      uint32_t dmaFullQ;
      uint32_t adcSync;
    };
  };
};

#endif
