#ifndef Dti_Module_hh
#define Dti_Module_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Cphw {
    class HsRepeater;
  }
  namespace Dti {

    class Stats;

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
      uint32_t  _reserved_12;
    };

    class Module {
    public:
      enum { NUsLinks=7 };
      enum { NDsLinks=7 };
    public:
      static class Module* module();
      static Pds::Cphw::HsRepeater* hsRepeater();
    public:
      Module();
    public:
      Stats stats() const;
    public:
      unsigned usLinkUp(        ) const;
      void     usLinkUp(unsigned);
      bool     bpLinkUp(        ) const;
      void     bpLinkUp(bool    );
      unsigned dsLinkUp(        ) const;
      void     dsLinkUp(unsigned);
    public:
      void usLink(unsigned) const; // Callable from const methods
      void dsLink(unsigned) const; // Callable from const methods
      void clearCounters()  const; // Callable from const methods
      void updateCounters() const; // Callable from const methods
    public:
      unsigned monClkRate(unsigned) const;
      bool     monClkSlow(unsigned) const;
      bool     monClkFast(unsigned) const;
      bool     monClkLock(unsigned) const;
    public:
      //  0x0000 - RW: Upstream link control
      UsLink      _usLink[NUsLinks];
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
      Cphw::Reg   _linkIndices;
      //  0x0080 - RO: Indexed upstream link - rx errors count
      Cphw::Reg   _usRxErrs;
      //  0x0084 - RO: Indexed upstream link - rx full count
      Cphw::Reg   _usRxFull;
      //  0x0088 - RO: Indexed upstream link - rx words count
      Cphw::Reg   _usIbRecv;
      //  0x008C - RO: Indexed upstream link - rx events count
      Cphw::Reg   _usIbEvt;
      //  0x0090 - RO: Indexed downstream link - rx errors count
      Cphw::Reg   _dsRxErrs;
      //  0x0094 - RO: Indexed downstream link - rx full count
      Cphw::Reg   _dsRxFull;
      //  0x0098 - RO: Indexed downstream link - tx words count
      //  [47:0]   obSent
      Cphw::Reg64 _dsObSent;
      //  0x00A0 - RO: Indexed upstream link - rx control words count
      Cphw::Reg   _usObRecv;
    private:
      uint32_t    _reserved_164;
    public:
      //  0x00A8 - RO: Indexed upstream link - tx control words count
      Cphw::Reg   _usObSent;
      //  0x00B0 - RO: Lock status of QPLLs
      //  [1:0]    qpllLock
      Cphw::Reg   _qpllLock;
      //  0x00B4 - RO: Monitor clock status
      //  [28:0]   monClkRate  Monitor clock rate
      //  [29]     monClkSlow  Monitor clock too slow
      //  [30]     monClkFast  Monitor clock too fast
      //  [31]     monClkLock  Monitor clock locked
      Cphw::Reg   _monClk[4];
      //  0x00C0 - RO: Count of outbound L0 triggers
      //  [19:0]   usStatus.obL0
      Cphw::Reg   _usLinkObL0;
      //  0x00C4 - RO: Count of outbound L1A triggers
      //  [19:0]   usStatus.obL1A
      Cphw::Reg   _usLinkObL1A;
      //  0x00C8 - RO: Count of outbound L1R triggers
      //  [19:0]   usStatus.obL1R
      Cphw::Reg   _usLinkObL1R;
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
    };
  };
};

#endif

