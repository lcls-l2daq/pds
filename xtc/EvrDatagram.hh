#ifndef EvrDatagram_hh
#define EvrDatagram_hh

#include "pdsdata/xtc/Sequence.hh"
#include "pds/xtc/L1AcceptEnv.hh"

namespace Pds {

  class EvrDatagram {
  public:
    EvrDatagram() {}
    EvrDatagram(const Sequence& _seq, unsigned _evr, unsigned _ncmds=0) : 
      seq(_seq), env(0), evr(_evr), ncmds(_ncmds) {}
    ~EvrDatagram() {}
    
  public:  
    void setL1AcceptEnv(unsigned int uReadoutGroup) { env = uReadoutGroup; }
  public:
    Sequence    seq;
    L1AcceptEnv env;
    unsigned    evr;
    unsigned    ncmds;
  };

}

#endif
