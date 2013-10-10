#ifndef EvrDatagram_hh
#define EvrDatagram_hh

#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/L1AcceptEnv.hh"

#include <stdint.h>

namespace Pds {

  class EvrDatagram {
  public:
    EvrDatagram() {}
    EvrDatagram(const Sequence& _seq, unsigned _evr, unsigned _ncmds=0) : 
      seq(_seq), env(0), evr(_evr), ncmds(_ncmds) {}
    ~EvrDatagram() {}
    
  public:  
    void setL1AcceptEnv(unsigned int uReadoutGroup) { env = L1AcceptEnv(uReadoutGroup); }
  public:
    Sequence    seq;
    L1AcceptEnv env;
    uint32_t    evr;
    uint32_t    ncmds;
  };

}

#endif
