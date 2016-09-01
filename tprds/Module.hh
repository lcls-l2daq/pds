#ifndef TprDS_Module_hh
#define TprDS_Module_hh

#include "pds/tpr/Module.hh"
#include "evgr/evr/evr.hh"

#include <unistd.h>
#include <string>

namespace Pds {
  namespace TprDS {
    class TprBase {
    public:
      enum { NCHANNELS=12 };
      enum { NTRIGGERS=12 };
      enum Destination { Any };
      enum FixedRate { _1M, _500K, _100K, _10K, _1K, _100H, _10H, _1H };
    public:
      void dump() const;
      void setupRate   (unsigned i,
                        unsigned rate,
                        unsigned dataLength);
      void setupDaq    (unsigned i,
                        unsigned partition,
                        unsigned dataLength);
      void disable     (unsigned i);
    public:
      volatile uint32_t irqEnable;
      volatile uint32_t irqStatus;;
      volatile uint32_t partitionAddr;
      volatile uint32_t gtxDebug;
      volatile uint32_t countReset;
      volatile uint32_t dmaFullThr; // [32-bit words]
      volatile uint32_t reserved_18;
      volatile uint32_t reserved_1C;
      struct {  // 0x20
        volatile uint32_t control;
        volatile uint32_t evtSel;
        volatile uint32_t evtCount;
        volatile uint32_t testData;  // [32-bit words]
        volatile uint32_t reserved[4];
      } channel[NCHANNELS];
      volatile uint32_t reserved_20[2];
      volatile uint32_t frameCount;
      volatile uint32_t pauseCount;
      volatile uint32_t overflowCount;
      volatile uint32_t idleCount;
      volatile uint32_t reserved_38[1];
      volatile uint32_t reserved_b[1+(14-NCHANNELS)*8];
      struct { // 0x200
        volatile uint32_t control; // input, polarity, enabled
        volatile uint32_t delay;
        volatile uint32_t width;
        volatile uint32_t reserved_t;
      } trigger[NTRIGGERS];
    };

    // Memory map of TPR registers (EvrCardG2 BAR 1)
    class TprReg {
    public:
      TprBase         base;
      volatile uint32_t reserved_0    [(0x400-sizeof(TprBase))/4];
      Tpr::DmaControl dma;
      volatile uint32_t reserved_1    [(0x40000-0x400-sizeof(Tpr::DmaControl))/4];
      Tpr::TprCore    tpr;
      volatile uint32_t reserved_tpr  [(0x10000-sizeof(Tpr::TprCore))/4];
      Tpr::RingB      ring0;
      volatile uint32_t reserved_ring0[(0x10000-sizeof(Tpr::RingB))/4];
      Tpr::RingB      ring1;
      volatile uint32_t reserved_ring1[(0x10000-sizeof(Tpr::RingB))/4];
      Tpr::TpgMini    tpg;
    };
  };
};

#endif
