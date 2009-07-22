#include "Transition.hh"

#include <string.h>
#include <time.h>

using namespace Pds;

static Sequence now(TransitionId::Value id)
{
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  unsigned pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
  ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
  TimeStamp timestamp(0, pulseId, 0);
  return Sequence(Sequence::Event, id, clocktime, timestamp);
}

Transition::Transition(TransitionId::Value id,
                       Phase           phase,
                       const Sequence& sequence,
                       const Env&      env, 
                       unsigned        size) :
  Message(Message::Transition, size),
  _id      (id),
  _phase   (phase),
  _sequence(sequence),
  _env     (env)
{}

Transition::Transition(TransitionId::Value id,
		       const Env&          env, 
		       unsigned            size) :
  Message  (Message::Transition, size),
  _id      (id),
  _phase   (Execute),
  _sequence(now(id)),
  _env     (env)
{}

Transition::Transition(const Transition& c) :
  Message(Message::Transition, c.reply_to(), c.size()),
  _id      (c._id),
  _phase   (c._phase),
  _sequence(c._sequence),
  _env     (c._env)
{
  memcpy(this+1,&c+1,c.size()-sizeof(Transition));
}

TransitionId::Value Transition::id() const {return _id;}

Transition::Phase Transition::phase() const {return _phase;}

const Sequence& Transition::sequence() const { return _sequence; }

const Env& Transition::env() const {return _env;}

void* Transition::operator new(size_t size, Pool* pool)
{
  return pool->alloc(size);
}

void* Transition::operator new(size_t size)
{
  PoolEntry* entry = 
    reinterpret_cast<PoolEntry*>(::new char[size+sizeof(PoolEntry)]);
  entry->_pool = 0;
  return entry+1;
}

void Transition::operator delete(void* buffer)
{
  PoolEntry* entry = PoolEntry::entry(buffer);
  if (entry->_pool) {
    Pool::free(buffer);
  } else {
    delete [] reinterpret_cast<char*>(entry);
  }
}

Allocation::Allocation() :
  _partitionid(0),
  _nnodes     (0)
{
}

Allocation::Allocation(const char* partition,
		       const char* dbpath,
		       unsigned    partitionid) : 
  _partitionid(partitionid),
  _nnodes     (0)
{
  strncpy(_partition, partition, MaxName-1);
  strncpy(_dbpath   , dbpath   , MaxDbPath-1);
}

bool Allocation::add(const Node& node)
{
  if (_nnodes < MaxNodes) {
    _nodes[_nnodes++] = node;
    return true;
  } else {
    return false;
  }
}

unsigned Allocation::partitionid() const { return _partitionid; }

unsigned Allocation::nnodes() const {return _nnodes;}

const Node* Allocation::node(unsigned n) const 
{
  if (n < _nnodes) {
    return _nodes+n;
  } else {
    return 0;
  }
}

const char* Allocation::partition() const {return _partition;}

const char* Allocation::dbpath() const {return _dbpath;}

unsigned    Allocation::size() const { return sizeof(*this)+(_nnodes-MaxNodes)*sizeof(Node); }


Allocate::Allocate(const Allocation& allocation) :
  Transition(TransitionId::Map, Transition::Execute, now(TransitionId::Map), 0,
             sizeof(Allocate)+allocation.size()-sizeof(Allocation)),
  _allocation(allocation)
{
}

Allocate::Allocate(const Allocation& allocation,
		   const Sequence& seq) :
  Transition(TransitionId::Map, Transition::Execute, seq, 0,
             sizeof(Allocate)+allocation.size()-sizeof(Allocation)),
  _allocation(allocation)
{
}

const Allocation& Allocate::allocation() const
{ return _allocation; }


Kill::Kill(const Node& allocator) : 
  Transition(TransitionId::Unmap, Transition::Execute, Sequence(), 0, 
             sizeof(Kill)),
  _allocator(allocator)
{}

const Node& Kill::allocator() const 
{
  return _allocator;
}
