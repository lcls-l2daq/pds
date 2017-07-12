#ifndef Xpm_MasterSim_hh
#define Xpm_MasterSim_hh

namespace Pds {
  namespace Xpm {

    class MasterSim {
    public:
      MasterSim();
      void init();
    public:
      bool     l0Enabled () const;
      L0Stats  l0Stats   () const;
      unsigned txLinkStat() const;
      unsigned rxLinkStat() const;
    public:
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
      Cphw::Reg   _dsLinkEnable;    
      Cphw::Reg   _dsLinkReset;
      Cphw::Reg   _enabled;
      Cphw::Reg   _l0Select;
      uint32_t    _reserved_10[(0x80-0x10)/4];
      Cphw::Reg   _dsTxLinkStatus;    
      Cphw::Reg   _dsRxLinkStatus;
      Cphw::Reg64 _l0Enabled;    
      Cphw::Reg64 _l0Inhibited;
      Cphw::Reg64 _numl0;
      Cphw::Reg64 _numl0Inh;
      Cphw::Reg64 _numl0Acc;
      Cphw::Reg   _rxLinkErrs[8];
      Cphw::Reg   _analysisRst;
      Cphw::Reg   _analysisTag; // [0:15] tag, [16:31] mask
    };
  };
};

#endif

