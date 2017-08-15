#ifndef Xpm_Module_hh
#define Xpm_Module_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"
#include "pds/cphw/AmcTiming.hh"
#include "pds/cphw/HsRepeater.hh"

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
    public:
      enum { NAmcs=2 };
      enum { NDSLinks=14 };
      enum { NPartitions=16 };
    public:
      static class Module* locate();
    public:
      Module();
      void init();
    public:
      Pds::Cphw::AmcTiming  _timing;
    private:
      uint32_t _reserved_AT[(0x09000000-sizeof(Module::_timing))>>2];
    public:
      Pds::Cphw::HsRepeater _hsRepeater[6];
    private:
      uint32_t _reserved_HR[(0x77000000-sizeof(Module::_hsRepeater))>>2];
    public:
      CoreCounts counts    () const;
      bool       l0Enabled () const;
      L0Stats    l0Stats   () const;
      unsigned   txLinkStat() const;
      unsigned   rxLinkStat() const;
    public:
      void clearLinks  ();
    public:
      unsigned rxLinkErrs(unsigned) const;
    public:
      void resetL0     (bool);
      void setL0Enabled(bool);
      bool getL0Enabled() const;
      void setL0Select_FixedRate(unsigned rate);
      void setL0Select_ACRate   (unsigned rate, unsigned tsmask);
      void setL0Select_Sequence (unsigned seq , unsigned bit);
      //      void setL0Select_EventCode(unsigned code);
      void lockL0Stats (bool);
      //    private:
    public:
      void setRingBChan(unsigned);
    public:
      void dumpPll     (unsigned) const;
      void pllBwSel    (unsigned, unsigned);
      void pllFrqTbl   (unsigned, unsigned);
      void pllFrqSel   (unsigned, unsigned);
      void pllRateSel  (unsigned, unsigned);
      void pllPhsInc   (unsigned);
      void pllPhsDec   (unsigned);
      void pllBypass   (unsigned, bool);
      void pllReset    (unsigned);
      unsigned pllBwSel  (unsigned) const;
      unsigned pllFrqTbl (unsigned) const;
      unsigned pllFrqSel (unsigned) const;
      unsigned pllRateSel(unsigned) const;
      bool     pllBypass (unsigned) const;
      unsigned pllStatus0(unsigned) const;
      unsigned pllCount0 (unsigned) const;
      unsigned pllStatus1(unsigned) const;
      unsigned pllCount1 (unsigned) const;
      void pllSkew       (unsigned, int);
    public:
      // Indexing
      void setPartition(unsigned) const;
      void setLink     (unsigned) const;
      void setLinkDebug(unsigned) const;
      void setAmc      (unsigned) const;
      void setInhibit  (unsigned);
      void setTagStream(unsigned);
      unsigned getPartition() const;
      unsigned getLink     () const;
      unsigned getLinkDebug() const;
      unsigned getAmc      () const;
      unsigned getInhibit  () const;
      unsigned getTagStream() const;
    public:
      void     linkTxDelay(unsigned, unsigned);
      unsigned linkTxDelay(unsigned) const;
      void     linkPartition(unsigned, unsigned);
      unsigned linkPartition(unsigned) const;
      void     linkTrgSrc(unsigned, unsigned);
      unsigned linkTrgSrc(unsigned) const;
      void     linkLoopback(unsigned, bool);
      bool     linkLoopback(unsigned) const;
      void     txLinkReset (unsigned);
      void     rxLinkReset (unsigned);
      void     linkEnable  (unsigned, bool);
      bool     linkEnable  (unsigned) const;
    public:
      void     setL1TrgClr(unsigned, unsigned);
      unsigned getL1TrgClr(unsigned) const;
      void     setL1TrgEnb(unsigned, unsigned);
      unsigned getL1TrgEnb(unsigned) const;
      void     setL1TrgSrc(unsigned, unsigned);
      unsigned getL1TrgSrc(unsigned) const;
      void     setL1TrgWord(unsigned, unsigned);
      unsigned getL1TrgWord(unsigned) const;
      void     setL1TrgWrite(unsigned, unsigned);
      unsigned getL1TrgWrite(unsigned) const;
    public:
      void     messageHdr(unsigned, unsigned);
      unsigned messageHdr(unsigned) const;
      void     messageIns(unsigned, unsigned);
      unsigned messageIns(unsigned) const;
    public:
      void     inhibitInt(unsigned, unsigned);
      unsigned inhibitInt(unsigned) const;
      void     inhibitLim(unsigned, unsigned);
      unsigned inhibitLim(unsigned) const;
      void     inhibitEnb(unsigned, unsigned);
      unsigned inhibitEnb(unsigned) const;
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
      //  [3:0]   bwSel         Loop filter bandwidth selection
      //  [5:4]   frqTbl        Frequency table - character {L,H,M} = 00,01,1x
      //  [15:8]  frqSel        Frequency selection - 4 characters
      //  [19:16] rate          Rate - 2 characters
      //  [20]    inc           Increment phase
      //  [21]    dec           Decrement phase
      //  [22]    bypass        Bypass PLL
      //  [23]    rstn          Reset PLL (inverted)
      //  [26:24] pllCount[0]   PLL count[0] for AMC[index] (RO)
      //  [27]    pllStat[0]    PLL stat[0]  for AMC[index] (RO)
      //  [30:28] pllCount[1]   PLL count[1] for AMC[index] (RO)
      //  [31]    pllStat[1]    PLL stat[1]  for AMC[index] (RO)
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
      //  [15:14]=01 (ac rate),    [8:3] timeslot mask, [2:0] ac rate marker
      //  [15:14]=10 (sequence),   [13:8] sequencer, [3:0] sequencer bit
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
      //  [3:0]   trigsrc       L1 trigger source link
      //  [12:4]  trigword      L1 trigger word
      //  [16]    trigwr        L1 trigger write mask
      Cphw::Reg   _l1config1;
      //  0x0054 - RW: Analysis tag reset for partition[index]
      //  [3:0]   reset
      Cphw::Reg   _analysisRst;
      //  0x0058 - RW: Analysis tag for partition[index]
      //  [31:0]  tag[3:0]
      Cphw::Reg   _analysisTag;
      //  0x005c - RW: Analysis push for partition[index]
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
      //  [11:0]  interval      interval (929kHz ticks)
      //  [15:12] limit         max # accepts within interval
      //  [31]    enable        enable
      Cphw::Reg    _inhibitConfig[4];
      //  0x0090 - RO: Inhibit assertions by DS link for partition[index]
      Cphw::Reg    _inhibitCounts[32];
    public:
      //  0x0110 - RO: Monitor clock
      //  [28: 0]  Rate
      //  [29]     Slow
      //  [30]     Fast
      //  [31]     Lock
      Cphw::Reg _monClk[4];
    };
  };
};

#endif

