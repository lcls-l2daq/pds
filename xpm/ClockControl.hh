#ifndef Xpm_ClockControl_hh
#define Xpm_ClockControl_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm {

    class ClockControl {
    public:
      ClockControl();
      void init();
      void dump();
      //    private:
    public:
      // AxiSpi
      uint32_t    _reserved_0[3];
      Cphw::Reg   _deviceType;
      Cphw::Reg   _prodUpper;
      Cphw::Reg   _prodLower;
      Cphw::Reg   _maskRev;
      uint32_t    _reserved_1C[0xC-0x7];
      Cphw::Reg   _vndrUpper;
      Cphw::Reg   _vndrLower;
      uint32_t    _reserved_38[0x100-0xE];
      Cphw::Reg   _lmkReg[0x7D];
      uint32_t    _reserved[0x100000/4-0x17D];
      uint32_t    _reserved_100000[0x200/4];
      // Axi
      Cphw::Reg   _lmkClkSel;
      Cphw::Reg   _lmkRst;
      Cphw::Reg   _lmkSync;
      Cphw::Reg   _lmkStatus;
      uint32_t    _reserved_100210;
      Cphw::Reg   _lmkMuxSel;
    };
  };
};

#endif

