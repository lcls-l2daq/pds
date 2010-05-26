#ifndef EbDump_hh
#define EbDump_hh

#include "pds/utility/Appliance.hh"
#include "pds/utility/EbBase.hh"
#include "pds/xtc/InDatagram.hh"

namespace Pds {
  class EbDump : public Appliance {
  public:
    EbDump(EbBase* eb) : _eb(eb) {}
  public:
    Transition* transitions(Transition* in) { return in; }
    InDatagram* occurrences(InDatagram* in) { return in; }
    InDatagram* events     (InDatagram* in) {
      const Datagram& dg = in->datagram();
      if (dg.seq.service()==TransitionId::Disable)
	_eb->dump(1);
      return in;
    }
  private:
    EbBase* _eb;
  };
};

#endif
