#ifndef LusiDiagFex_hh
#define LusiDiagFex_hh

#include "pds/ipimb/IpimbFex.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/service/GenericPoolW.hh"

#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"

#include <vector>
#include <map>

namespace Pds {
  class InDatagram;
  class Src;
  class CfgClientNfs;
  class Transition;
  class IpimbCapSetting;

  class LusiDiagFex : public IpimbFex, XtcIterator {
  public:
    LusiDiagFex(int baselineMode, const std::map<uint32_t,int>& polarity);
    ~LusiDiagFex();
  public:
    void        reset          ();
    bool        configure      (CfgClientNfs&, Transition&, const IpimbConfigType&);
    void        recordConfigure(InDatagram*, const Src&);
    InDatagram* process        (InDatagram*);
  public:
    int process(Xtc*);
  private:
    int                    _baselineMode;
    std::map<uint32_t,int> _polarity;
    GenericPoolW           _pool;
    IpmFexConfigType*      _ipm_config;
    DiodeFexConfigType*    _pim_config;
    IpimbCapSetting*       _cap_config;
    InDatagram*            _odg;
    std::vector<unsigned>  _map;
  };
};

#endif

