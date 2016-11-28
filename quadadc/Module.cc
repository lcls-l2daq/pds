#include "pds/quadadc/Module.hh"

#include <stdio.h>

using namespace Pds::QuadAdc;

enum 
{
        FMC12X_INTERNAL_CLK = 0,                                                /*!< FMC12x_init() configure the FMC12x for internal clock operations */
        FMC12X_EXTERNAL_CLK = 1,                                                /*!< FMC12x_init() configure the FMC12x for external clock operations */
        FMC12X_EXTERNAL_REF = 2,                                                /*!< FMC12x_init() configure the FMC12x for external reference operations */

        FMC12X_VCXO_TYPE_2500MHZ = 0,                                   /*!< FMC12x_init() Vco on the card is 2.5GHz */
        FMC12X_VCXO_TYPE_2200MHZ = 1,                                   /*!< FMC12x_init() Vco on the card is 2.2GHz */
        FMC12X_VCXO_TYPE_2000MHZ = 2,                                   /*!< FMC12x_init() Vco on the card is 2.0GHz */
};

enum 
{
        CLOCKTREE_CLKSRC_EXTERNAL = 0,                                                  /*!< FMC12x_clocktree_init() configure the clock tree for external clock operations */
        CLOCKTREE_CLKSRC_INTERNAL = 1,                                                  /*!< FMC12x_clocktree_init() configure the clock tree for internal clock operations */
        CLOCKTREE_CLKSRC_EXTREF   = 2,                                                  /*!< FMC12x_clocktree_init() configure the clock tree for external reference operations */

        CLOCKTREE_VCXO_TYPE_2500MHZ = 0,                                                /*!< FMC12x_clocktree_init() Vco on the card is 2.5GHz */
        CLOCKTREE_VCXO_TYPE_2200MHZ = 1,                                                /*!< FMC12x_clocktree_init() Vco on the card is 2.2GHz */
        CLOCKTREE_VCXO_TYPE_2000MHZ = 2,                                                /*!< FMC12x_clocktree_init() Vco on the card is 2.0GHz */
        CLOCKTREE_VCXO_TYPE_1600MHZ = 3,                                                /*!< FMC12x_clocktree_init() Vco on the card is 1.6 GHZ */
        CLOCKTREE_VCXO_TYPE_BUILDIN = 4,                                                /*!< FMC12x_clocktree_init() Vco on the card is the AD9517 build in VCO */
        CLOCKTREE_VCXO_TYPE_2400MHZ = 5                                                 /*!< FMC12x_clocktree_init() Vco on the card is 2.4GHz */
};

enum 
{       // clock sources
        CLKSRC_EXTERNAL_CLK = 0,                                                /*!< FMC12x_cpld_init() external clock. */
        CLKSRC_INTERNAL_CLK_EXTERNAL_REF = 3,                   /*!< FMC12x_cpld_init() internal clock / external reference. */
        CLKSRC_INTERNAL_CLK_INTERNAL_REF = 6,                   /*!< FMC12x_cpld_init() internal clock / internal reference. */

        // sync sources
        SYNCSRC_EXTERNAL_TRIGGER = 0,                                   /*!< FMC12x_cpld_init() external trigger. */
        SYNCSRC_HOST = 1,                                                               /*!< FMC12x_cpld_init() software trigger. */
        SYNCSRC_CLOCK_TREE = 2,                                                 /*!< FMC12x_cpld_init() signal from the clock tree. */
        SYNCSRC_NO_SYNC = 3,                                                    /*!< FMC12x_cpld_init() no synchronization. */

        // FAN enable bits
        FAN0_ENABLED = (0<<4),                                                  /*!< FMC12x_cpld_init() FAN 0 is enabled */
        FAN1_ENABLED = (0<<5),                                                  /*!< FMC12x_cpld_init() FAN 1 is enabled */
        FAN2_ENABLED = (0<<6),                                                  /*!< FMC12x_cpld_init() FAN 2 is enabled */
        FAN3_ENABLED = (0<<7),                                                  /*!< FMC12x_cpld_init() FAN 3 is enabled */
        FAN0_DISABLED = (1<<4),                                                 /*!< FMC12x_cpld_init() FAN 0 is disabled */
        FAN1_DISABLED = (1<<5),                                                 /*!< FMC12x_cpld_init() FAN 1 is disabled */
        FAN2_DISABLED = (1<<6),                                                 /*!< FMC12x_cpld_init() FAN 2 is disabled */
        FAN3_DISABLED = (1<<7),                                                 /*!< FMC12x_cpld_init() FAN 3 is disabled */

        // LVTTL bus direction (HDMI connector)
        DIR0_INPUT      = (0<<0),                                                       /*!< FMC12x_cpld_init() DIR 0 is input */
        DIR1_INPUT      = (0<<1),                                                       /*!< FMC12x_cpld_init() DIR 1 is input */
        DIR2_INPUT      = (0<<2),                                                       /*!< FMC12x_cpld_init() DIR 2 is input */
        DIR3_INPUT      = (0<<3),                                                       /*!< FMC12x_cpld_init() DIR 3 is input */
        DIR0_OUTPUT     = (1<<0),                                                       /*!< FMC12x_cpld_init() DIR 0 is output */
        DIR1_OUTPUT     = (1<<1),                                                       /*!< FMC12x_cpld_init() DIR 1 is output */
        DIR2_OUTPUT     = (1<<2),                                                       /*!< FMC12x_cpld_init() DIR 2 is output */
        DIR3_OUTPUT     = (1<<3),                                                       /*!< FMC12x_cpld_init() DIR 3 is output */
};

void Module::init()
{
  fmca_spi.initSPI();
}

void Module::fmc_init()
{
// if(FMC12x_init(AddrSipFMC12xBridge, AddrSipFMC12xClkSpi, AddrSipFMC12xAdcSpi, AddrSipFMC12xCpldSpi, AddrSipFMC12xAdcPhy, 
//                modeClock, cardType, GA, typeVco, carrierKC705)!=FMC12X_ERR_OK) {

#ifdef TIMINGREF
  const uint32_t clockmode = FMC12X_EXTERNAL_REF;
#else
  const uint32_t clockmode = FMC12X_INTERNAL_CLK;
#endif

  uint32_t clksrc_cpld;
  uint32_t clksrc_clktree;
  uint32_t vcotype = 0; // default 2500 MHz

  if(clockmode==FMC12X_INTERNAL_CLK) {
    clksrc_cpld    = CLKSRC_INTERNAL_CLK_INTERNAL_REF;
    clksrc_clktree = CLOCKTREE_CLKSRC_INTERNAL;
  }
  else if(clockmode==FMC12X_EXTERNAL_REF) {
    clksrc_cpld    = CLKSRC_INTERNAL_CLK_EXTERNAL_REF;
    clksrc_clktree = CLOCKTREE_CLKSRC_EXTREF;
  }
  else {
    clksrc_cpld    = CLKSRC_EXTERNAL_CLK;
    clksrc_clktree = CLOCKTREE_CLKSRC_EXTERNAL;
  }

  if (fmca_spi.cpld_init(clksrc_cpld))
    printf("cpld_init failed\n");

  if (fmca_spi.clocktree_init(clksrc_clktree, vcotype))
    printf("clocktree_init failed\n");
}

int Module::train_io()
{
  //
  //  IO Training
  //
  if (fmca_spi.adc_enable_test(Flash11))
    printf("adc_enable_test failed\n");

  adc_core.start_training();

  if (fmca_spi.adc_disable_test())
    printf("adc_disable_test failed\n");

  if (fmca_spi.adc_enable_test(Flash11))
    printf("adc_enable_test failed\n");

  adc_core.loop_checking();

  if (fmca_spi.adc_disable_test())
    printf("adc_disable_test failed\n");

  return 0;
}

void Module::enable_test_pattern(TestPattern p)
{
  if (p < 8) {
    if (fmca_spi.adc_enable_test(p))
      printf("adc_enable_test failed\n");
  }
  else
    base.enableDmaTest(true);
}

void Module::disable_test_pattern()
{
  if (fmca_spi.adc_disable_test())
    printf("adc_disable_test failed\n");
  base.enableDmaTest(false);
}

#if 0
void Module::setClockLCLS  (unsigned delay_int, unsigned delay_frac)
{
  base.resetClock(true);
  mmcm.setLCLS(delay_int, delay_frac);
  base.resetClock(false);
}

void Module::setClockLCLSII(unsigned delay_int, unsigned delay_frac)
{
  base.resetClock(true);
  mmcm.setLCLSII(delay_int, delay_frac);
  base.resetClock(false);
  // check for locked bit
}
#endif

void Module::setRxAlignTarget(unsigned t)
{
  unsigned v = gthAlignTarget;
  v &= ~0x3f;
  v |= (t&0x3f);
  gthAlignTarget = v;
}

void Module::setRxResetLength(unsigned len)
{
  unsigned v = gthAlignTarget;
  v &= ~0xf0000;
  v |= (len&0xf)<<16;
  gthAlignTarget = v;
}

void Module::dumpRxAlign     () const
{
  printf("\nTarget: %u\tRstLen: %u\tLast: %u\n",
         gthAlignTarget&0x7f,
         (gthAlignTarget>>16)&0xf, 
         gthAlignLast&0x7f);
  for(unsigned i=0; i<128; i++) {
    printf(" %04x",(gthAlign[i/2] >> (16*(i&1)))&0xffff);
    if ((i%10)==9) printf("\n");
  }
  printf("\n");
}

