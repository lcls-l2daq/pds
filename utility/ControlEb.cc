#include "pds/utility/ControlEb.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/Task.hh"

using namespace Pds;

ControlEb::ControlEb(const Node& n, Routine* tmo, int iDuration) :
  _hdr(n),
  _timeout(tmo),
  _task(new Task(TaskObject("cntleb"))),
  _alloc(0),
  _buffer(sizeof(Allocate),8),
  _duration(iDuration)
{
}

ControlEb::~ControlEb()
{
  _task->destroy();
}

void        ControlEb::reset(const Allocation& alloc)
{
  _alloc = &alloc;
  _previous = _remaining;
  _remaining.clearAll();
  for(unsigned k=0; k<alloc.nnodes(); k++)
    if (alloc.node(k)->level() < Pds::Level::Observer)
      _remaining.setBit(k);
  for(unsigned k=0; k<alloc.nnodes(); k++)
    if (alloc.node(k)->level() == Pds::Level::Segment) {
      _master = *alloc.node(k);
      break;
    }
  cancel();
  start();
}

Transition* ControlEb::build(const Node& hdr,
                             const Transition& tr)
{
  cancel();

  //  if (hdr == _hdr)
  if (hdr == _master) {
    if (_buffer.numberOfAllocatedObjects()) {
      printf("ControlEb::build tr %d  seq %08x.%08x depth %d\n",
             tr.id(), 
             tr.sequence().clock().seconds(),
             tr.sequence().clock().nanoseconds(),
             _buffer.numberOfAllocatedObjects());
    }
    _pending = new(&_buffer) Transition(tr);
  }

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

//#ifdef BUILD_PRINCETON
//unsigned ControlEb::duration() const { return 60000; }
//#elif defined(BUILD_READOUT_GROUP)
//unsigned ControlEb::duration() const { return 10000; }
//#else
//unsigned ControlEb::duration() const { return 5000; }
//#endif
unsigned ControlEb::duration() const { return _duration; }

unsigned ControlEb::repetitive() const { return 0; }

Allocation ControlEb::remaining() const
{
  Allocation alloc(_alloc->partition(),
		   _alloc->dbpath(),
		   _alloc->l3path(),
		   _alloc->partitionid());
  for(unsigned k=0; k<_alloc->nnodes(); k++) {
    if (_remaining.hasBitSet(k))
      alloc.add(*_alloc->node(k));
  }
  return alloc;
}

Transition* ControlEb::recover()
{
  printf("ControlEb::recover pending %s.%d  remaining/previous %08x/%08x\n",
         _pending ? TransitionId::name(_pending->id()) : "None",
         _pending ? int(_pending->phase()) : -1,
         _remaining.value(),_previous.value());
  for(unsigned k=0; k<_alloc->nnodes(); k++)
    if (_remaining.hasBitSet(k))
      printf("  %08x.%d\n",_alloc->node(k)->ip(), _alloc->node(k)->pid());

  if (_pending) {
    if ((_remaining & _previous).isZero()) {
      return _pending;
    }
  }
  return 0;
}
