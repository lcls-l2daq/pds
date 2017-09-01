#ifndef Dti_Module_hh
#define Dti_Module_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"
#include "pds/cphw/AmcTiming.hh"
#include "pds/cphw/HsRepeater.hh"

namespace Pds {
  namespace Dti {

    class Stats;

    class Module {
    public:
      enum { NUsLinks=7 };
      enum { NDsLinks=7 };
    public:
      static class Module* locate();
    public:
      Module();
      void init();
    public:
      Stats stats() const;
    public:
      Pds::Cphw::AmcTiming  _timing;
    private:
      uint32_t _reserved_AT[(0x09000000-sizeof(Module::_timing))>>2];
    public:
      Pds::Cphw::HsRepeater _hsRepeater[6];
    private:
      uint32_t _reserved_HR[(0x77000000-sizeof(Module::_hsRepeater))>>2];
    public:
      unsigned usLinkUp(        ) const;
      void     usLinkUp(unsigned);
      bool     bpLinkUp(        ) const;
      void     bpLinkUp(bool    );
      unsigned dsLinkUp(        ) const;
      void     dsLinkUp(unsigned);
      unsigned usLinkEnabled(unsigned idx) const;
      void     usLinkEnabled(unsigned idx, unsigned v);
      unsigned usLinkTrigDelay(unsigned idx) const;
      void     usLinkTrigDelay(unsigned idx, unsigned v);
      unsigned usLinkFwdMask(unsigned idx) const;
      void     usLinkFwdMask(unsigned idx, unsigned v);
    public:
      unsigned usLink()         const;
      void     usLink(unsigned) const; // Callable from const methods
      unsigned dsLink()         const;
      void     dsLink(unsigned) const; // Callable from const methods
      void     clearCounters()  const; // Callable from const methods
      void     updateCounters() const; // Callable from const methods
    public:
      unsigned qpllLock()     const;
      unsigned bpTxInterval() const;
      void     bpTxInterval(unsigned qpllLock);
    public:
      unsigned monClkRate(unsigned) const;
      bool     monClkSlow(unsigned) const;
      bool     monClkFast(unsigned) const;
      bool     monClkLock(unsigned) const;
    public:
      //  0x0000 - RW: Upstream link control
      class UsLink {
      public:
        bool     enabled   () const;
        bool     tagEnabled() const;
        bool     l1Enabled () const;
        unsigned partition () const;
        unsigned trigDelay () const;
        unsigned fwdMask   () const;
        bool     fwdMode   () const;
      public:
        void enable   (bool    );
        void tagEnable(bool    );
        void l1Enable (bool    );
        void partition(unsigned);
        void trigDelay(unsigned);
        void fwdMask  (unsigned);
        void fwdMode  (bool    );
      public:
        //  0x0000 - RW: Link config
        //  [0]      enable      Enable link
        //  [1]      tagEnable   Enable link event tag support
        //  [2]      l1Enable    Enable L1 trigger support
        //  [7:4]    partition   Upstream link partition
        //  [15:8]   trigDelay   Timing frame trigger delay
        //  [28:16]  fwdMask     Upstream link forwarding mask
        //  [31]     fwdMode     Upstream link forwarding mode (RR, RR+full)
        Cphw::Reg   _config;
        //  0x0004 - RW: Data source identifier
        Cphw::Reg   _dataSource;
        //  0x0008 - RW: Data type identifier
        Cphw::Reg   _dataType;
      private:
        uint32_t    _reserved_12;
      }           _usLinkConfig[NUsLinks];
      //  0x0070 - RO: Link up statuses
      //  [6:0]    usLinkUp    Upstream link status
      //  [15]     bpLinkUp    Backplane link status
      //  [22:16]  dsLinkUp    Downstream link status
      Cphw::Reg   _linkUp;
      //  0x0074 - RW: Link indices and counter control
      //  [3:0]    usLink      Upstream link index
      //  [19:16]  dsLink      Downstream link index
      //  [30]     clear       Clear link counters
      //  [31]     update      Update link counters
      Cphw::Reg   _index;
      //  0x0078 - RO: Tx words count
      Cphw::Reg   _bpObSent;
    private:
      uint32_t    _reserved_124;
    public:
      class UsStatus
      {
      public:
        unsigned rxErrs()    const;
        unsigned remLinkId() const;
      public:
        //  0x0080 - RO:
        //  [23:0]   rxErrs    Indexed upstream link - rx errors count
        //  [31:24]  remLinkID Indexed upstream link - remote link ID
        Cphw::Reg   _rxErrs;
        //  0x0084 - RO: Indexed upstream link - rx full count
        Cphw::Reg   _rxFull;
        ////  0x0088 - RO: Indexed upstream link - rx words count
        //Cphw::Reg   _ibRecv;
        //  0x0088 - RO: Indexed upstream link - rx events dropped
        Cphw::Reg   _ibDump;
        //  0x008C - RO: Indexed upstream link - rx events forwarded
        Cphw::Reg   _ibEvt;
      }           _usStatus;
      class DsStatus
      {
      public:
        unsigned rxErrs()    const;
        unsigned remLinkId() const;
      public:
        //  0x0090 - RO:
        //  [23:0]   rxErrs    Indexed downstream link - rx errors count
        //  [31:24]  remLinkID Indexed downstream link - remote link ID
        Cphw::Reg   _rxErrs;
        //  0x0094 - RO: Indexed downstream link - rx full count
        Cphw::Reg   _rxFull;
        //  0x0098 - RO: Indexed downstream link - tx words count
        //  [47:0]   obSent
        Cphw::Reg64 _obSent;
      }           _dsStatus;
      //  0x00A0 - RO: Indexed upstream link - rx control words count
      Cphw::Reg   _usObRecv;
    private:
      uint32_t    _reserved_164;
    public:
      //  0x00A8 - RO: Indexed upstream link - tx control words count
      Cphw::Reg   _usObSent;
    private:
      uint32_t    _reserved_172;
    public:
      //  0x00B0 - RO:
      //  [1:0]    qpllLock    Lock status of QPLLs
      //  [23:16]  bpPeriod    rxFull push interval - 125 MHz ticks
      Cphw::Reg   _qpllLock;
      //  0x00B4 - RO: Monitor clock status
      //  [28:0]   monClkRate  Monitor clock rate
      //  [29]     monClkSlow  Monitor clock too slow
      //  [30]     monClkFast  Monitor clock too fast
      //  [31]     monClkLock  Monitor clock locked
      Cphw::Reg   _monClk[4];
#if 0                            // These don't exist in the f/w right now
      //  0x00C4 - RO: Count of outbound L0 triggers
      //  [19:0]   usStatus.obL0
      Cphw::Reg   _usLinkObL0;
      //  0x00C8 - RO: Count of outbound L1A triggers
      //  [19:0]   usStatus.obL1A
      Cphw::Reg   _usLinkObL1A;
      //  0x00CC - RO: Count of outbound L1R triggers
      //  [19:0]   usStatus.obL1R
      Cphw::Reg   _usLinkObL1R;
#else
    private:
      uint32_t    _reserved_196[3];
#endif
    private:
      uint32_t    _reserved_208[(0x10000000-208)>>2];
    public:
      class Pgp2bAxi
      {
      public:
        Cphw::Reg   _countReset;        // 0xd4
      private:
        uint32_t    _reserved[16];      // 0xd8
      public:
        Cphw::Reg   _rxFrameErrs;       // 0x118
        Cphw::Reg   _rxFrames;          // 0x11c
      private:
        uint32_t    _reserved2[4];      // 0x120
      public:
        Cphw::Reg   _txFrameErrs;       // 0x130
        Cphw::Reg   _txFrames;          // 0x134
      private:
        uint32_t    _reserved3[5];      // 0x138
      public:
        Cphw::Reg   _txOpcodes;         // 0x14c
        Cphw::Reg   _rxOpcodes;         // 0x150
      private:
        uint32_t    _reserved4[0x80>>2]; // 0x154 - 0x1d0
      public:
        void clearCounters() const;    // Const so that we can call it from const methods
      }           _pgp[2];               // Revisit: Only US[0] and DS[0] for now
    private:
      uint32_t    _reserved_724[(0x10000000-724)>>2];
    public:
      // 0x2d8 - RO
      class TheRingBuffer : public Cphw::RingBuffer
      {
      public:
        void acqNdump();
      }           _ringBuffer;
    };

    class Stats {
    public:
      void dump() const;
    public:
      // Revisit: Useful?  unsigned enabled;
      // Revisit: Useful?  unsigned tagEnabled;
      // Revisit: Useful?  unsigned l1Enabled;

      unsigned usLinkUp;
      unsigned bpLinkUp;
      unsigned dsLinkUp;
      struct
      {
        unsigned rxErrs;
        unsigned rxFull;
        unsigned ibRecv;
        unsigned ibEvt;
        unsigned obRecv;
        unsigned obSent;
      }        us[Module::NUsLinks];
      unsigned bpObSent;
      struct
      {
        unsigned rxErrs;
        unsigned rxFull;
        uint64_t obSent;
      }        ds[Module::NDsLinks];
      unsigned qpllLock;
      struct
      {
        unsigned rate;
        unsigned slow;
        unsigned fast;
        unsigned lock;
      }        monClk[4];
      unsigned usLinkObL0;
      unsigned usLinkObL1A;
      unsigned usLinkObL1R;
      struct
      {
         unsigned rxFrameErrs;
         unsigned rxFrames;
         unsigned txFrameErrs;
         unsigned txFrames;
         unsigned txOpcodes;
         unsigned rxOpcodes;
      }        pgp[2];
    };
  };
};

#endif

