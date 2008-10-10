#ifndef Datagram_hh
#define Datagram_hh

#include "Sequence.hh"
#include "Env.hh"
#include "InXtc.hh"
#include "TypeId.hh"

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
      seq(), env(0), xtc(ctn,src) {}

    //  Create a datagram from a transition
    Datagram(const Transition& tr, const TypeId& type, const Src& src) :
      seq(tr.sequence()), env(tr.env()), xtc(type,src) {}

    void* operator new(size_t size, Pds::Pool* pool)
    { return pool->alloc(size); }

    void operator delete(void* buffer)
    { RingPool::free(buffer); }


    Sequence seq;
    Env   env;
    InXtc xtc;
  };

}

#endif
