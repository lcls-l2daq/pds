#ifndef PDS_OUTLETWIRE_HH
#define PDS_OUTLETWIRE_HH

namespace Pds {

  class Datagram;
  class Transition;
  class InDatagram;
  class Occurrence;
  class Outlet;
  class Ins;

  class OutletWire {
  public:
    OutletWire(Outlet& outlet);
    virtual ~OutletWire();

    virtual Transition* forward(Transition* dg ) = 0;
    virtual Occurrence* forward(Occurrence* occ) = 0;
    virtual InDatagram* forward(InDatagram* dg ) = 0;

    enum NamedConnection { Bcast };
    virtual void bind(NamedConnection, const Ins& ) = 0;
    virtual void bind(unsigned id, const Ins& node) = 0;
    virtual void unbind(unsigned id) = 0;

    // Debugging
    virtual void dump(int detail) {}
    virtual void dumpCounters(int detail) {}
    virtual void resetCounters() {}
    virtual void dumpHistograms(unsigned tag, const char* path) {}
    virtual void resetHistograms() {}

  protected:
    void _log(const Datagram& dg, int result);
    void _log(const Datagram& dg, const Ins& ins, int result);
  };
}
#endif
