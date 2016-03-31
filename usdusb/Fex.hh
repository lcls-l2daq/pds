#ifndef Pds_UsdUsb_Fex_hh
#define Pds_UsdUsb_Fex_hh

#include "pds/config/UsdUsbFexConfigType.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
    class Xtc;
    class InDatagram;
    class CfgClientNfs;
    class Transition;

  namespace UsdUsb {
    class Fex : public XtcIterator {
    public:
      Fex(CfgClientNfs&);
      ~Fex();
    public:
      bool        configure      (Transition&);
      void        recordConfigure(InDatagram*);
      InDatagram* process        (InDatagram*);
    public:
      int process(Xtc*);
    private:
      CfgClientNfs&          _cfg;
      UsdUsbFexConfigType*   _usdusb_config;
      InDatagram*            _in;
    };
  };
};

#endif
