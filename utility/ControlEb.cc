#include "pds/utility/ControlEb.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/Task.hh"

using namespace Pds;

ControlEb::ControlEb(const Node& n, Routine* tmo) :
  _hdr(n),
  _timeout(tmo),
  _task(new Task(TaskObject("cntleb"))),
  _alloc(0),
  _buffer(sizeof(Allocate),1)
{
}

ControlEb::~ControlEb() 
{
  _task->destroy();
}

void        ControlEb::reset(const Allocation& alloc)
{
  _alloc = &alloc;
  _remaining.clearAll();
  _remaining = ~_remaining;
  _remaining >>= (_remaining.BitMaskBits - alloc.nnodes());
  cancel();
  start();
}

Transition* ControlEb::build(const Node& hdr,
			     const Transition& tr)
{
  cancel();

  if (hdr == _hdr)
    _pending = new(&_buffer) Transition(tr);

  for(unsigned k=0; k<_alloc->nnodes(); k++) {
    if (hdr == *_alloc->node(k)) {
      _remaining.clearBit(k);
      if (_remaining.isZero()) return _pending;
      else break;
    }	
  }

  start();
  return 0;
}

void ControlEb::expired() {   
  if (_timeout) _timeout->routine(); 
}

Task* ControlEb::task() { return _task; }
unsigned ControlEb::duration() const { return 3000; }
unsigned ControlEb::repetitive() const { return 0; }

Allocation ControlEb::remaining() const
{
  Allocation alloc(_alloc->partition(),
		   _alloc->dbpath(),
		   _alloc->partitionid());
  for(unsigned k=0; k<_alloc->nnodes(); k++) {
    if (_remaining.hasBitSet(k))
      alloc.add(*_alloc->node(k));
  }
  return alloc;
}
