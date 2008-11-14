#ifndef Pds_Datagram_hh
#define Pds_Datagram_hh

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/Env.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"

#include "pds/service/Pool.hh"
#include "pds/service/RingPool.hh"
#include "pds/service/RingPoolW.hh"

#include "pds/utility/Transition.hh"

namespace Pds {

  class Datagram {
  public:

    Datagram() {}

    //  Copy the header
    Datagram(const Datagram& dg) :
      seq(dg.seq), env(dg.env), xtc(dg.xtc) {}

    //  Replace the TypeId and Src
    Datagram(const Datagram& dg, const TypeId& ctn, const Src& src) :
      seq(dg.seq), env(dg.env), xtc(ctn,src) {}

    //  Create an empty datagram with identifiers
    Datagram(const TypeId& ctn, const Src& src) :
      seq(Sequence::Event,TransitionId::Unknown,ClockTime(0,0),0,0), 
      env(0), xtc(ctn,src) {}

    //  Create a datagram from a transition
    Datagram(const Transition& tr, const TypeId& type, const Src& src) :
      seq(tr.sequence()), env(tr.env()), xtc(type,src) {}

    void* operator new(size_t size, Pds::Pool* pool)
    { return pool->alloc(size); }

    void operator delete(void* buffer)
    { RingPool::free(buffer); }

    PDS_DGRAM_STRUCT;
  };

}

#endif
