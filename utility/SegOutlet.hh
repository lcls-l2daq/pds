#ifndef Pds_SegOutlet_hh
#define Pds_SegOutlet_hh

#include "pds/utility/OutletWire.hh"
#include "pds/service/Client.hh"

namespace Pds {
  class CollectionManager;

  class SegOutlet : public OutletWire {
  public:
    SegOutlet(Outlet& outlet, 
              CollectionManager&,
              const Ins&);
    ~SegOutlet();
    Transition* forward(Transition* dg);
    Occurrence* forward(Occurrence* dg);
    InDatagram* forward(InDatagram* dg);
    void bind(NamedConnection, const Ins& node);
    void bind(unsigned id, const Ins& node);
    void unbind(unsigned id);
  private:
    CollectionManager& _collection;
    Client             _client;
    const Ins&         _occurrences;
    Ins                _datagrams;
  };
};

#endif
