#ifndef Pds_InDatagram_hh
#define Pds_InDatagram_hh

#include "pds/xtc/Datagram.hh"

namespace Pds {
  class InDatagram : public Datagram {
  public:
    InDatagram(const Datagram& dg) : Datagram(dg) {}

    Datagram& datagram() { return *this; }
    const Datagram& datagram() const { return *this; }
  };
};

#endif
