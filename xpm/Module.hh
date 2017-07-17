#ifndef Xpm_Module_hh
#define Xpm_Module_hh

#include "pds/cphw/AmcTiming.hh"

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm {

    class CoreCounts {
    public:
      void dump() const;
    public:
      uint64_t rxClkCount;
      uint64_t txClkCount;
      uint64_t rxRstCount;
      uint64_t crcErrCount;
      uint64_t rxDecErrCount;
      uint64_t rxDspErrCount;
      uint64_t bypassResetCount;
      uint64_t bypassDoneCount;
      uint64_t rxLinkUp;
      uint64_t fidCount;
      uint64_t sofCount;
      uint64_t eofCount;
    };

    class Core : public Pds::Cphw::AmcTiming {
    public:
      static class Core& get();
    public:
      CoreCounts counts() const;
    };

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
      unsigned txLinkStat(unsigned) const;
      unsigned rxLinkStat(unsigned) const;
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
      unsigned pllStatus0() const;
      unsigned pllCount0() const;
      unsigned pllStatus1() const;
      unsigned pllCount1() const;
      void pllSkew     (int);
    public:
      // Indexing
      void setPartition(unsigned);
      void setLink     (unsigned) const;
      void setLinkDebug(unsigned);
      void setAmc      (unsigned);
      void setInhibit  (unsigned);
      void setTagStream(unsigned);
      unsigned getPartition() const;
      unsigned getLink     () const;
      unsigned getLinkDebug() const;
      unsigned getAmc      () const;
      unsigned getInhibit  () const;
      unsigned getTagStream() const;
    public:
      //  0x0000 - RO: physical link address
      Cphw::Reg   _paddr;
      //  0x0004 - RW: programming index
      //  [3:0]   partition     Partition number
      //  [9:4]   link          Link number
      //  [14:10] linkDebug     Link number for input to ring buffer
      //  [16]    amc           AMC selection
      //  [21:20] inhibit       Inhibit index
      //  [24]    tagStream     Enable tag FIFO streaming input
      Cphw::Reg   _index;
      //  0x0008 - RW: ds link configuration for link[index]
      //  [19:0]  txDelay       Transmit delay
      //  [23:20] partition     Partition
      //  [27:24] trigsrc       Trigger source
      //  [28]    loopback      Loopback mode
      //  [29]    txReset       Transmit reset
      //  [30]    rxReset       Receive  reset
      //  [31]    enable        Enable
      Cphw::Reg  _dsLinkConfig;
      //  0x000C - RO: ds link status for link[index]
      //  [15:0]  rxErrCnts     Receive  error counts
      //  [16]    txResetDone   Transmit reset done
      //  [17]    txReady       Transmit ready
      //  [18]    rxResetDone   Receive  reset done
      //  [19]    rxReady       Receive  ready
      //  [20]    rxIsXpm       Remote side is XPM
      Cphw::Reg  _dsLinkStatus;
      //  0x0010 - RW: PLL configuration for AMC[index]
      //  [3:0]   loop filter bandwidth selection
      //  [5:4]   frequency table - character {L,H,M} = 00,01,1x
      //  [15:8]  frequency selection - 4 characters
      //  [19:16] rate - 2 characters
      //  [20]    increment phase
      //  [21]    decrement phase
      //  [22]    bypass PLL
      //  [23]    reset PLL (inverted)
      //  [26:24] pllcount[0] for amc[index] (RO)
      //  [27]    pll stat[0] for amc[index] (RO)
      //  [30:28] pllcount[1] for amc[index] (RO)
      //  [31]    pll stat[1] for amc[index] (RO)
      Cphw::Reg   _pllConfig;
      //  0x0014 - RW: L0 selection control for partition[index]
      //  [0]     reset
      //  [16]    enable
      //  [31]    enable counter update
      Cphw::Reg   _l0Control;
      //  0x0018 - RW: L0 selection criteria for partition[index]
      //  [15: 0]  rateSel      L0 rate selection
      //  [31:16]  destSel      L0 destination selection
      //
      //  [15:14]=00 (fixed rate), [3:0] fixed rate marker
      //  [15:14]=01 (ac rate), [8:3] timeslot mask, [2:0] ac rate marker
      //  [15:14]=10 (sequence), [13:8] sequencer, [3:0] sequencer bit
      //  [31]=1 any destination or match any of [14:0] mask of destinations
      Cphw::Reg   _l0Select;
      //  0x001c - RO: Clks enabled for partition[index]
      Cphw::Reg64 _l0Enabled;
      //  0x0024 - RO: Clks inhibited for partition[index]
      Cphw::Reg64 _l0Inhibited;
      //  0x002c - RO: Num L0s input for partition[index]
      Cphw::Reg64 _numl0;
      //  0x0034 - RO: Num L0s inhibited for partition[index]
      Cphw::Reg64 _numl0Inh;
      //  0x003c - RO: Num L0s accepted for partition[index]
      Cphw::Reg64 _numl0Acc;
      //  0x0044 - RO: Num L1s accepted for partition[index]
      Cphw::Reg64 _numl1Acc;
      //  0x004c - RW: L1 select config for partition[index]
      //  [0]     NL1Triggers clear  mask bits
      //  [16]    NL1Triggers enable mask bits
      Cphw::Reg   _l1config0;
      //  0x0050 - RW: L1 select config for partition[index]
      //  [3:0]   src           L1 trigger source link
      //  [12:4]  word          L1 trigger word
      //  [16]    wr            L1 trigger write mask
      Cphw::Reg   _l1config1;
      //  0x0054 - RW: Analysis tag reset for partition[index]
      //  [3:0]   reset
      Cphw::Reg   _analysisRst;
      //  0x0058 - RW: Analysis tag reset for partition[index]
      //  [31:0]  tag[3:0]
      Cphw::Reg   _analysisTag;
      //  0x005c - RW: Analysis push reset for partition[index]
      //  [3:0]   push
      Cphw::Reg   _analysisPush;
      //  0x0060 - RO: Analysis tag push counts for partition[index]
      Cphw::Reg   _analysisTagWr;
      //  0x0064 - RO: Analysis tag pull counts for partition[index]
      Cphw::Reg   _analysisTagRd;
      //  0x0068 - RW: Pipeline depth for partition[index]
      Cphw::Reg   _pipelineDepth;
      //  0x006c - RW: Message setup for partition[index]
      //  [14: 0]  Header
      //  [15]     Insert
      Cphw::Reg   _message;
      //  0x0070 - RW: Message payload for partition[index]
      Cphw::Reg   _messagePayload;
    private:
      uint32_t    _reserved_116[3];
    public:
      //  0x0080 - RW: Inhibit configurations for partition[index]
      //  [11:0]  interval (929kHz ticks)
      //  [15:12] max # accepts within interval
      //  [31]    enable
      Cphw::Reg    _inhibitConfig[4];
      //  0x0090 - RO: Inhibit assertions by DS link for partition[index]
      Cphw::Reg    _inhibitCounts[NDSLinks];
    private:
      uint32_t    _reserved_148_268[32-NDSLinks];
    public:
      //  0x0110 - RO: Monitor clock
      //  [28: 0]  Rate
      //  [29]     Lock
      //  [30]     Slow
      //  [31]     Fast
      Cphw::Reg _monClk[4];
    };
  };
};

#endif

