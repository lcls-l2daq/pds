#ifndef QuadAdc_Module_hh
#define QuadAdc_Module_hh

#include "pds/tprds/Module.hh"
#include "pds/quadadc/ClkSynth.hh"
#include "pds/quadadc/Mmcm.hh"
#include "pds/quadadc/DmaCore.hh"
#include "pds/quadadc/PhyCore.hh"
#include "pds/quadadc/RingBuffer.hh"
#include "pds/quadadc/I2cSwitch.hh"
#include "pds/quadadc/LocalCpld.hh"
#include "pds/quadadc/FmcSpi.hh"
#include "pds/quadadc/QABase.hh"
#include "pds/quadadc/Adt7411.hh"
#include "pds/quadadc/AdcCore.hh"
#include "pds/quadadc/AdcSync.hh"
#include "pds/quadadc/FmcCore.hh"
#include "pds/quadadc/FlashController.hh"

#include <unistd.h>
#include <string>

namespace Pds {
  namespace QuadAdc {

    class Module {
    public:
      //
      //  High level API
      //
      
      //  Initialize busses
      void init();

      //  Initialize clock tree and IO training
      void fmc_init();

      int  train_io();

      enum TestPattern { Ramp=0, Flash11=1, Flash12=3, Flash16=5, DMA=8 };
      void enable_test_pattern(TestPattern);
      void disable_test_pattern();

      //      void setClockLCLS  (unsigned delay_int, unsigned delay_frac);
      //      void setClockLCLSII(unsigned delay_int, unsigned delay_frac);

      void setRxAlignTarget(unsigned);
      void setRxResetLength(unsigned);
      void dumpRxAlign     () const;

      //
      //  Low level API
      //
    public:
      Pds::Tpr::AxiVersion version;
    private:
      uint32_t rsvd_to_0x08000[(0x8000-sizeof(Pds::Tpr::AxiVersion))/4];
    public:
      FlashController      flash;
    private:
      uint32_t rsvd_to_0x10000[(0x8000-sizeof(FlashController))/4];
    public:
      I2cSwitch i2c_sw_control;  // 0x10000
      ClkSynth  clksynth;
      LocalCpld local_cpld;
      Adt7411   vtmon1;
      Adt7411   vtmon2;
      Adt7411   vtmon3;
      Adt7411   vtmona;
      FmcSpi    fmca_spi;
      FmcSpi    fmcb_spi;
      uint32_t rsvd_to_0x20000[(0x10000-13*0x400)/4];
    private:

      // DMA
    public:
      DmaCore           dma_core; // 0x20000
      uint32_t rsvd_to_0x30000[(0x10000-sizeof(DmaCore))/4];

      // PHY
      PhyCore           phy_core; // 0x30000
      uint32_t rsvd_to_0x40000[(0x10000-sizeof(PhyCore))/4];

      // Timing
    public:
      Pds::Tpr::TprCore  tpr;     // 0x40000
    private:
      uint32_t rsvd_to_0x50000  [(0x10000-sizeof(Pds::Tpr::TprCore))/4];
    public:
      RingBuffer         ring0;   // 0x50000
    private:
      uint32_t rsvd_to_0x60000  [(0x10000-sizeof(RingBuffer))/4];
    public:
      RingBuffer         ring1;   // 0x60000
    private:
      uint32_t rsvd_to_0x70000  [(0x10000-sizeof(RingBuffer))/4];
    private:
      uint32_t rsvd_to_0x80000  [0x10000/4];

      //  App registers
    public:
      QABase   base;             // 0x80000
      uint32_t rsvd_to_0x80800  [(0x800-sizeof(QABase))/4];
    public:
      Mmcm     mmcm;             // 0x80800
      FmcCore  fmc_core;         // 0x81000
      AdcCore  adc_core;         // 0x81400
      uint32_t rsvd_to_0x82000  [(0x800)/4];
      AdcSync  adc_sync;
      uint32_t rsvd_to_0x90000  [(0xE000-sizeof(AdcSync))/4];
    public:
      uint32_t gthAlign[64];     // 0x90000
      uint32_t gthAlignTarget;
      uint32_t gthAlignLast;
    };
  };
};

#endif
