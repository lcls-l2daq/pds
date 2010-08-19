#ifndef LusiDiagFex_hh
#define LusiDiagFex_hh

#include "pds/ipimb/IpimbFex.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/service/GenericPool.hh"

#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"

namespace Pds {
  class InDatagram;
  class Src;
  class CfgClientNfs;
  class Transition;
  class IpimbCapSetting;

  class LusiDiagFex : public IpimbFex, XtcIterator {
  public:
    LusiDiagFex();
    ~LusiDiagFex();
  public:
    bool        configure      (CfgClientNfs&, Transition&, const IpimbConfigType&);
    void        recordConfigure(InDatagram*, const Src&);
    InDatagram* process        (InDatagram*);
  public:
    int process(Xtc*);
  private:
    GenericPool            _pool;
    IpmFexConfigType*      _ipm_config;
    DiodeFexConfigType*    _pim_config;
    IpimbCapSetting*       _cap_config;
    InDatagram*            _odg;
  };
};

#endif

