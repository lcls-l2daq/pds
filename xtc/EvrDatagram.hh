#ifndef EvrDatagram_hh
#define EvrDatagram_hh

#include "pdsdata/xtc/Sequence.hh"

namespace Pds {

  class EvrDatagram {
  public:
    EvrDatagram() {}
    EvrDatagram(const Sequence& _seq,
		unsigned _evr,
                unsigned _ncmds=0) : 
      seq(_seq), evr(_evr), ncmds(_ncmds) {}
    ~EvrDatagram() {}
  public:
    Sequence seq;
    unsigned evr;
    unsigned ncmds;
  };

}

#endif
