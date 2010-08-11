#ifndef IpimbFex_hh
#define IpimbFex_hh

#include "pds/config/IpimbConfigType.hh"

namespace Pds {
  class InDatagram;
  class Src;
  class CfgClientNfs;
  class Transition;

  class IpimbFex {
  public:
    virtual ~IpimbFex() {}
  public:
    virtual bool        configure      (CfgClientNfs&, Transition&, const IpimbConfigType&) { return true; }
    virtual void        recordConfigure(InDatagram*, const Src&)    {}
    virtual InDatagram* process        (InDatagram* in)             { return in; }
  };
};

#endif

