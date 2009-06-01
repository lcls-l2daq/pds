#include "pds/utility/ControlEb.hh"
#include "pds/utility/Transition.hh"

using namespace Pds;

ControlEb::ControlEb(const Node& n) :
  _hdr(n),
  _alloc(0),
  _buffer(sizeof(Allocate),1)
{
}

ControlEb::~ControlEb() 
{
}

void        ControlEb::reset(const Allocation& alloc)
{
  _alloc = &alloc;
  _remaining.clearAll();
  _remaining = ~_remaining;
  _remaining >>= (_remaining.BitMaskBits - alloc.nnodes());
}

Transition* ControlEb::build(const Node& hdr,
			     const Transition& tr)
{
  if (hdr == _hdr)
    _pending = new(&_buffer) Transition(tr);

  for(unsigned k=0; k<_alloc->nnodes(); k++) {
    if (hdr == *_alloc->node(k)) {
      _remaining.clearBit(k);
      return (_remaining.isZero() ? _pending : 0);
    }	
  }

  return 0;
}
