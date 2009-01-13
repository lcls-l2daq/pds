#ifndef EvrDatagram_hh
#define EvrDatagram_hh

#include "pdsdata/xtc/Sequence.hh"

namespace Pds {

  class EvrDatagram {
  public:
    EvrDatagram() {}
    EvrDatagram(const Sequence& _seq,
		unsigned _evr) : 
      seq(_seq), evr(_evr) {}
    ~EvrDatagram() {}

    Sequence seq;
    unsigned evr;
  };

}

#endif
