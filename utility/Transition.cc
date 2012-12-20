#include "Transition.hh"

#include <string.h>
#include <time.h>
#include <stdio.h>

using namespace Pds;

static Sequence now(TransitionId::Value id)
{
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  unsigned pulseId = -1;
  ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
  TimeStamp timestamp(0, pulseId, 0);
  return Sequence(Sequence::Event, id, clocktime, timestamp);
}

static Sequence none(TransitionId::Value id)
{
  return Sequence(Sequence::Event, id, ClockTime(-1, -1), TimeStamp(0,-1,0) );
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
  _sequence(none(id)),
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

TransitionId::Value Transition::id() const {return TransitionId::Value(_id);}

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

void Transition::_stampIt()
{
  _sequence = now(id());
}

Allocation::Allocation() :
  _partitionid(0),
  _nnodes     (0)
{
  _bld_mask[0] = _bld_mask[1] = 0;
}

Allocation::Allocation(const char* partition,
                       const char* dbpath,
                       unsigned    partitionid,
                       uint64_t    bld_mask) : 
  _partitionid(partitionid),
  _nnodes     (0)
{
  strncpy(_partition, partition, MaxName-1);
  strncpy(_dbpath   , dbpath   , MaxDbPath-1);
  _bld_mask[1] = (bld_mask>>32)&0xffffffff;
  _bld_mask[0] = (bld_mask>> 0)&0xffffffff;
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

Node* Allocation::node(unsigned n)
{
  if (n < _nnodes) {
    return _nodes+n;
  } else {
    return 0;
  }
}

unsigned Allocation::nnodes(Level::Type level) const
{
  unsigned n=0;
  for(unsigned i=0; i<_nnodes; i++)
    if (_nodes[i].level()==level)
      n++;
  return n;
}

uint64_t Allocation::bld_mask() const 
{
  uint64_t mask = _bld_mask[1];
  mask = (mask<<32) | _bld_mask[0];
  return mask;
}

const char* Allocation::partition() const {return _partition;}

const char* Allocation::dbpath() const {return _dbpath;}

unsigned    Allocation::size() const { return sizeof(*this)+(_nnodes-MaxNodes)*sizeof(Node); }


Allocate::Allocate(const Allocation& allocation) :
  Transition(TransitionId::Map, Transition::Execute, none(TransitionId::Map), 0,
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

RunInfo::RunInfo(unsigned run, unsigned experiment) :
  Transition(TransitionId::BeginRun, Transition::Execute,
             none(TransitionId::BeginRun), run,
             sizeof(RunInfo)),
  _run(run),_experiment(experiment)
{
  _expname[0] = '\0';
}
RunInfo::RunInfo(unsigned run, unsigned experiment, char *expname) :
  Transition(TransitionId::BeginRun, Transition::Execute,
             none(TransitionId::BeginRun), run,
             sizeof(RunInfo)),
  _run(run),_experiment(experiment)
{
  strncpy(_expname, expname, MaxExpName-1);
}
unsigned RunInfo::run() {return _run;}
unsigned RunInfo::experiment() {return _experiment;}
char *RunInfo::expname() {return _expname;}

Kill::Kill(const Node& allocator) : 
  Transition(TransitionId::Unmap, Transition::Execute, Sequence(), 0, 
             sizeof(Kill)),
  _allocator(allocator)
{}

const Node& Kill::allocator() const 
{
  return _allocator;
}
