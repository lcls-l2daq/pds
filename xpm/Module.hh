#ifndef Xpm_Module_hh
#define Xpm_Module_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm {

    class L0Stats {
    public:
      void dump() const;
    public:
      uint64_t l0Enabled;    
      uint64_t l0Inhibited;
      uint64_t numl0;
      uint64_t numl0Inh;
      uint64_t numl0Acc;
      uint32_t linkInh[8];
      uint16_t rx0Errs;
    };

    class Module {
    public:
      Module();
      void init();
    public:
      bool     l0Enabled () const;
      L0Stats  l0Stats   () const;
      unsigned txLinkStat() const;
      unsigned rxLinkStat() const;
    public:
      void clearLinks  ();
      void linkEnable  (unsigned, bool);
      void linkLoopback(unsigned, bool);
      void txLinkReset (unsigned);
      void rxLinkReset (unsigned);
    public:
      unsigned rxLinkErrs(unsigned) const;
    public:
      void resetL0     (bool);
      void setL0Enabled(bool);
      void setL0Select_FixedRate(unsigned rate);
      void setL0Select_ACRate   (unsigned rate, unsigned tsmask);
      void setL0Select_Sequence (unsigned seq , unsigned bit);
      //      void setL0Select_EventCode(unsigned code);
      void lockL0Stats (bool);
      //    private:
    public:
      void setRingBChan(unsigned);
    public:
      void pllBwSel    (unsigned);
      void pllFrqSel   (unsigned);
      void pllRateSel  (unsigned);
      void pllBypass   (bool);
      void pllReset    ();
      void pllSkew     (int);
    public:
      //  lower 16 bits enables a link to contribute to FULL
      //  upper 16 bits sets loopback mode for a link
      Cphw::Reg   _dsLinkEnable;
      //  lower 16 bits resets Tx for a link
      //  upper 16 bits resets Rx for a link
      Cphw::Reg   _dsLinkReset;
      //  lower 16 bits resets  L0 selection for a partition (only partition 0 for now)
      //  upper 16 bits enables L0 selection for a partition (only partition 0 for now)
      Cphw::Reg   _enabled;
      //  L0 selection criteria
      //    [15:14]=00 (fixed rate), [3:0] fixed rate marker
      //    [15:14]=01 (ac rate), [8:3] timeslot mask, [2:0] ac rate marker
      //    [15:14]=10 (sequence), [13:8] sequencer, [3:0] sequencer bit
      //    [31]=1 any destination or match any of [14:0] mask of destinations
      Cphw::Reg   _l0Select;
      //  AMC0 PLL configuration
      //    [3:0] loop filter bandwidth selection
      //    [5:4] frequency table - character {L,H,M} = 00,01,1x
      //    [15:8] frequency selection - 4 characters
      //    [19:16] rate - 2 characters
      //    [20] increment phase
      //    [21] decrement phase
      //    [22] bypass PLL
      //    [23] reset PLL
      //   ([26:24] input link number for AxiLiteRingBuffer)
      Cphw::Reg   _pllConfig0;
      //  AMC1 PLL configuration
      Cphw::Reg   _pllConfig1;
      //  AMC0 PLL status
      //    [14:0] loss-of-signal count
      //    [15]   current loss-of-signal
      //    [30:16] loss-of-lock count
      //    [31]   current loss-of-lock
      Cphw::Reg   _pllStatus0;
      //  AMC1 PLL status
      Cphw::Reg   _pllStatus1;
      //  L0 inhibit counts by link
      Cphw::Reg   _dsLinkInh[8];
      uint32_t    _reserved_1c[(0x80-0x40)/4];
      //  [15: 0] link tx PMA reset done
      //  [31:16] link tx complete reset done
      Cphw::Reg   _dsTxLinkStatus;   
      //  [15: 0] link rx PMA reset done
      //  [31:16] link rx complete reset done
      Cphw::Reg   _dsRxLinkStatus;
      //  Clks enabled
      Cphw::Reg64 _l0Enabled;    
      //  Clks inhibited
      Cphw::Reg64 _l0Inhibited;
      //  Num L0s input
      Cphw::Reg64 _numl0;
      //  Num L0s inhibited
      Cphw::Reg64 _numl0Inh;
      //  Num L0s accepted
      Cphw::Reg64 _numl0Acc;
      //  rx link error counts [link number]
      Cphw::Reg   _rxLinkErrs[8];
      //  [15:0] analysis tag bit FIFO reset
      Cphw::Reg   _analysisRst;
      //  [15: 0] analysis tag bit value
      //  [31:16] analysis tag bit write enable
      Cphw::Reg   _analysisTag; // [0:15] tag, [16:31] mask
    };
  };
};

#endif

