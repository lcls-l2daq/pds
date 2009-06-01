#ifndef Pds_ControlEb_hh
#define Pds_ControlEb_hh

#include "pds/service/BitMaskArray.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {

  class Node;
  class Transition;
  class Allocation;

  class ControlEb {
  public:
    ControlEb (const Node&);
    ~ControlEb();
  public:
    void        reset(const Allocation& alloc);
    Transition* build(const Node&, 
		      const Transition&);
  private:
    const Node&       _hdr;
    const Allocation* _alloc;
    GenericPool       _buffer;
    Transition*       _pending;
    enum { MAX_CONTRIBUTORS=128 };
    BitMaskArray<(MAX_CONTRIBUTORS>>5)> _remaining;
  };

};

#endif
    
