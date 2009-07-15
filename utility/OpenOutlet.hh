#ifndef Pds_OpenOutlet_hh
#define Pds_OpenOutlet_hh

#include "pds/utility/OutletWire.hh"

namespace Pds {
  class OpenOutlet : public OutletWire {
  public:
    OpenOutlet(Outlet& outlet) : OutletWire(outlet) {}
    virtual Transition* forward(Transition* dg) { return 0; }
    virtual Occurrence* forward(Occurrence* dg) { return 0; }
    virtual InDatagram* forward(InDatagram* dg) { return 0; }
    virtual void bind(NamedConnection, const Ins& node) {}
    virtual void bind(unsigned id, const Ins& node) {}
    virtual void unbind(unsigned id) {}
  };
};

#endif
