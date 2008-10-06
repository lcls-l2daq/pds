#include "Transition.hh"

#include <string.h>

using namespace Pds;

Transition::Transition(Id              id,
                       Phase           phase,
                       const Sequence& sequence,
                       unsigned        env, 
                       unsigned        size) :
  Message(Message::Transition, size),
  _id      (id),
  _phase   (phase),
  _sequence(sequence),
  _env     (env)
{}

Transition::Transition(Id              id,
                       unsigned        env, 
                       unsigned        size) :
  Message(Message::Transition, size),
  _id      (id),
  _phase   (Execute),
  _sequence(),
  _env     (env)
{}

Transition::Id Transition::id() const {return _id;}

Transition::Phase Transition::phase() const {return _phase;}

const Sequence& Transition::sequence() const { return _sequence; }

unsigned Transition::env() const {return _env;}

const char* Transition::name(Id id)
{ 
  static const char* _names[] = {
    "Map",
    "Unmap",
    "Configure",
    "Unconfigure",
    "BeginRun",
    "EndRun",
    "Enable",
    "Disable",
    "L1Accept"
  };
  return (id < NumberOf ? _names[id] : "-Invalid-");
};

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

Pds::Allocate::Allocate(const char* partition) : 
  Transition(Transition::Map, Transition::Execute, Sequence(), 0,
             sizeof(Allocate)-sizeof(_nodes)),
  _nnodes(0)
{
  strncpy(_partition, partition, MaxName-1);
}

bool Pds::Allocate::add(const Node& node)
{
  if (_nnodes < MaxNodes) {
    _nodes[_nnodes++] = node;
    _size += sizeof(Node);
    return true;
  } else {
    return false;
  }
}

unsigned Pds::Allocate::nnodes() const {return _nnodes;}

const Pds::Node* Pds::Allocate::node(unsigned n) const 
{
  if (n < _nnodes) {
    return _nodes+n;
  } else {
    return 0;
  }
}

const char* Pds::Allocate::partition() const {return _partition;}

Pds::Kill::Kill(const Node& allocator) : 
  Transition(Transition::Unmap, Transition::Execute, Sequence(), 0, 
             sizeof(Kill)),
  _allocator(allocator)
{}

const Pds::Node& Pds::Kill::allocator() const 
{
  return _allocator;
}
