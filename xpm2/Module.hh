#ifndef Xpm2_Module_hh
#define Xpm2_Module_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm2 {

    class L0Stats {
    public:
      void dump() const;
    public:
      uint64_t l0Enabled;    
      uint64_t l0Inhibited;
      uint64_t numl0;
      uint64_t numl0Inh;
      uint64_t numl0Acc;
      uint32_t linkInh[32];
      uint16_t rx0Errs;
    };

    class Module {
      enum { NAmcs=2 };
      enum { NDSLinks=14 };
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
      void dumpPll     () const;
      void pllBwSel    (unsigned);
      void pllFrqSel   (unsigned);
      void pllRateSel  (unsigned);
      void pllBypass   (bool);
      void pllReset    ();
      void pllSkew     (int);
    public:
      // Indexing
      void setPartition(unsigned);
      void setAmc      (unsigned);
      void setLink     (unsigned) const;
      unsigned getPartition() const;
      unsigned getAmc      () const;
      unsigned getLink     () const;
    public:
      //  physical link address
      Cphw::Reg   _paddr;
      //  programming index
      //  [3:0]   partition
      //  [7:4]   link
      //  [11:8]  linkDebug
      //  [12]    tagStream
      //  [16]    amc
      //  [21:20] inhibit
      Cphw::Reg   _index;
      //  ds link configuration
      //  [19:0]  txDelay
      //  [23:20] partition
      //  [27:24] trigsrc
      //  [28]    loopback
      //  [29]    txReset
      //  [30]    rxReset
      //  [31]    enable
      Cphw::Reg  _dsLinkConfig;
      //  ds link status
      //  [15:0]  rx error counts
      //  [16]    tx reset done
      //  [17]    tx ready
      //  [18]    rx reset done
      //  [19]    rx ready
      //  [20]    is xpm
      Cphw::Reg  _dsLinkStatus;
      //  AMC0 PLL configuration
      //    [3:0] loop filter bandwidth selection
      //    [5:4] frequency table - character {L,H,M} = 00,01,1x
      //    [15:8] frequency selection - 4 characters
      //    [19:16] rate - 2 characters
      //    [20] increment phase
      //    [21] decrement phase
      //    [22] bypass PLL
      //    [23] reset PLL
      //    [26:24] pllcount[0] for amc[index]
      //    [27]    pll stat[0] for amc[index]
      //    [30:28] pllcount[1] for amc[index]
      //    [31]    pll stat[1] for amc[index]
      Cphw::Reg   _pllConfig;
      //  L0 selection control
      //    [0]     reset
      //    [16]    enable
      //    [31]    enable counter update
      Cphw::Reg   _l0Control;
      //  L0 selection criteria for partition[index]
      //    [15:14]=00 (fixed rate), [3:0] fixed rate marker
      //    [15:14]=01 (ac rate), [8:3] timeslot mask, [2:0] ac rate marker
      //    [15:14]=10 (sequence), [13:8] sequencer, [3:0] sequencer bit
      //    [31]=1 any destination or match any of [14:0] mask of destinations
      Cphw::Reg   _l0Select;
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
      //  Num L1s accepted
      Cphw::Reg64 _numl1Acc;
      //  L1 select config
      //  [0]     clear    
      //  [16]    enable
      Cphw::Reg64 _l1config0;
      //  L1 select config
      //  [3:0]   src    
      //  [12:4]  word
      //  [16]    wr
      Cphw::Reg64 _l1config1;
      //  Analysis tag reset
      //  [3:0] reset
      Cphw::Reg   _analysisRst;
      //  Analysis tag
      //  [31:0] tag[3:0]
      Cphw::Reg   _analysisTag;
      //  Analysis push
      //  [3:0]  push
      Cphw::Reg   _analysisPush;
      //  
      Cphw::Reg   _analysisTagWr;
      Cphw::Reg   _analysisTagRd;
      Cphw::Reg   _pipelineDepth;
    private:
      uint32_t    _reserved_108[5];
    public:
      //  Inhibit configurations
      //  [11:0]  interval (929kHz ticks)
      //  [15:12] max # accepts within interval
      //  [31]    enable
      Cphw::Reg    _inhibitConfig[4];
      //  Inhibit assertions by DS link
      Cphw::Reg    _inhibitCounts[32];
    };
  };
};

#endif

