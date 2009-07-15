#ifndef PDS_TOEVENTWIRE_HH
#define PDS_TOEVENTWIRE_HH

#include <sys/uio.h>

#include "OutletWire.hh"
#include "ToNetEb.hh"

#include "OutletWireInsList.hh"

namespace Pds {

  class CollectionManager;
  class Outlet;
  class Datagram;

  class ToEventWire : public OutletWire {
  public:
    ToEventWire(Outlet& outlet,
		CollectionManager&,
		int interface,
		int maxbuf,
		const Ins& occurrences);
    ~ToEventWire();

    virtual Transition* forward(Transition* dg);
    virtual Occurrence* forward(Occurrence* dg);
    virtual InDatagram* forward(InDatagram* dg);
    virtual void bind(NamedConnection, const Ins& );
    virtual void bind(unsigned id, const Ins& node);
    virtual void unbind(unsigned id);
    
    // Debugging
    virtual void dump(int detail);
    virtual void dumpHistograms(unsigned tag, const char* path);
    virtual void resetHistograms();

    bool isempty() const {return _nodes.isempty();}
    
  private:
    OutletWireInsList  _nodes;
    CollectionManager& _collection;
    ToNetEb            _postman;
    Ins                _bcast;
    const Ins&         _occurrences;
  };
}

#endif
