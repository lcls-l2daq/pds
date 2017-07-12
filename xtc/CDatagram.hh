#ifndef Pds_CDatagram_hh
#define Pds_CDatagram_hh

#include "pds/xtc/InDatagram.hh"

namespace Pds {
  class CDatagram : public InDatagram {
  public:
    CDatagram(const Datagram& dg) : InDatagram(dg) {}
  };
};

#endif
