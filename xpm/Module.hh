#ifndef Xpm_Module_hh
#define Xpm_Module_hh

#include "Reg.hh"
#include "Reg64.hh"

namespace Xpm {

  class L0Stats {
  public:
    uint64_t l0Enabled;    
    uint64_t l0Inhibited;
    uint64_t numl0;
    uint64_t numl0Inh;
    uint64_t numl0Acc;
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
    void linkEnable (unsigned, bool);
    void txLinkReset(unsigned);
    void rxLinkReset(unsigned);
  public:
    void setL0Enabled(bool);
    void setL0Select_FixedRate(unsigned rate);
    void setL0Select_ACRate   (unsigned rate, unsigned tsmask);
    void setL0Select_Sequence (unsigned seq , unsigned bit);
  private:
    Reg   _dsLinkEnable;    
    Reg   _dsLinkReset;
    Reg   _enabled;
    Reg   _l0Select;
    uint32_t reserved_8[(0x80-0x10)/4];
    Reg   _dsTxLinkStatus;    
    Reg   _dsRxLinkStatus;
    Reg64 _l0Enabled;    
    Reg64 _l0Inhibited;
    Reg64 _numl0;
    Reg64 _numl0Inh;
    Reg64 _numl0Acc;
  };
};

#endif

