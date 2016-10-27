#ifndef QuadAdc_Module_hh
#define QuadAdc_Module_hh

#include "pds/tprds/Module.hh"
#include "pds/quadadc/ClkSynth.hh"

#include <unistd.h>
#include <string>

namespace Pds {
  namespace QuadAdc {

    class DmaCore {
    public:
      void init(unsigned maxDmaSize=0);
      void dump() const;
      uint32_t rxEnable;
      uint32_t txEnable;
      uint32_t fifoClear;
      uint32_t irqEnable;
      uint32_t fifoValid; // W fifoThres, R b0 = inbound, b1=outbound
      uint32_t maxRxSize; // inbound
      uint32_t mode;      // b0 = online, b1=acknowledge, b2=ibrewritehdr
      uint32_t irqStatus; // W b0=ack, R b0=ibPend, R b1=obPend
      uint32_t irqRequests;
      uint32_t irqAcks;
      uint32_t irqHoldoff;

      uint32_t reserved[245];

      uint32_t ibFifoPop;
      uint32_t obFifoPop;
      uint32_t reserved_pop[62];

      uint32_t loopFifoData; // RO
      uint32_t reserved_loop[63];

      uint32_t ibFifoPush[16];  // W data, R[0] status
      uint32_t obFifoPush[16];  // R b0=full, R b1=almost full, R b2=prog full
      uint32_t reserved_push[32];
    };

    class PhyCore {
    public:
      void dump() const;
    public:
      uint32_t rsvd_0[0x130/4];
      uint32_t bridgeInfo;
      uint32_t bridgeCSR;
      uint32_t irqDecode;
      uint32_t irqMask;
      uint32_t busLocation;
      uint32_t phyCSR;
      uint32_t rootCSR;
      uint32_t rootMSI1;
      uint32_t rootMSI2;
      uint32_t rootErrorFifo;
      uint32_t rootIrqFifo1;
      uint32_t rootIrqFifo2;
      uint32_t rsvd_160[2];
      uint32_t cfgControl;
      uint32_t rsvd_16c[(0x208-0x16c)/4];
      uint32_t barCfg[0x30/4];
      uint32_t rsvd_238[(0x1000-0x238)/4];
    };

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

    class QABase {
    public:
      void init();
      void setChannels(unsigned);
      enum Interleave { Q_NONE, Q_ABCD };
      void setMode    (Interleave);
      void setupDaq (unsigned partition,
                     unsigned length);
      void setupRate(unsigned rate,
                     unsigned length);
      void start();
      void stop ();
      void resetCounts();
      void dump() const;
    public:
      uint32_t irqEnable;
      uint32_t irqStatus;
      uint32_t partitionAddr;
      uint32_t dmaFullThr;
      uint32_t csr; // [31:31] acqEnable
      uint32_t acqSelect;
      // [3:0] channel enable mask
      // [8:8] interleave
      // [19:16] partition
      uint32_t control;
      uint32_t samples;       //  Must be a multiple of 4
      uint32_t prescale;
      uint32_t offset;        //  Not implemented
      uint32_t countAcquire;
      uint32_t countEnable;
      uint32_t countInhibit;
    };

    class Module {
    public:
      Pds::Tpr::AxiVersion version;
    private:
      uint32_t rsvd_to_0x10000[(0x10000-sizeof(version))/4];

      // I2C
    public:
      uint32_t  i2c_sw_control;
    private:
      uint32_t  _rsvd_i2c[255];
    public:
      ClkSynth  clksynth;
    private:
      uint32_t rsvd_to_0x20000[(0x10000-0x400-sizeof(clksynth))/4];

      // DMA
    public:
      DmaCore           dma_core;
      uint32_t rsvd_to_0x30000[(0x10000-sizeof(dma_core))/4];

      // PHY
      PhyCore           phy_core;
      uint32_t rsvd_to_0x40000[(0x10000-sizeof(phy_core))/4];

      // Timing
    public:
      Pds::Tpr::TprCore  tpr;
    private:
      uint32_t rsvd_to_0x50000  [(0x10000-sizeof(tpr))/4];
    public:
      RingBuffer         ring0;
    private:
      uint32_t rsvd_to_0x60000  [(0x10000-sizeof(ring0))/4];
    public:
      RingBuffer         ring1;
    private:
      uint32_t rsvd_to_0x70000  [(0x10000-sizeof(ring1))/4];
    private:
      uint32_t rsvd_to_0x80000  [0x10000/4];

      //  App registers
    public:
      QABase   base;
      volatile uint32_t    reserved_0    [(0x10000-sizeof(QABase))/4];

    public:
      uint32_t gthAlign[10];
      uint32_t gthAlignTarget;
    };
  };
};

#endif
