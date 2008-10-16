#ifndef PDSTRANSITION_HH
#define PDSTRANSITION_HH

#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"
#include "pds/xtc/Sequence.hh"
#include "pds/service/Pool.hh"
#include "TransitionId.hh"

namespace Pds {

class Transition : public Message {
public:
  enum Phase { Execute, Record };
  static const char* name(TransitionId::Value id);

public:
  Transition(TransitionId::Value id,
             Phase           phase,
             const Sequence& sequence,
             unsigned        env, 
             unsigned        size=sizeof(Transition));

  Transition(TransitionId::Value id,
             unsigned        env, 
             unsigned        size=sizeof(Transition));
  Transition(const Transition&);

  TransitionId::Value id      () const;
  Phase           phase   () const;
  const Sequence& sequence() const;
  unsigned        env     () const;

  void* operator new(size_t size, Pool* pool);
  void* operator new(size_t size);
  void  operator delete(void* buffer);

private:
  TransitionId::Value _id;
  Phase        _phase;
  Sequence     _sequence;
  unsigned     _env;
};

class Allocate : public Transition {
public:
  Allocate(const char* partition,
	   unsigned    partitionid);
  Allocate(const char* partition,
	   unsigned    partitionid,
	   const Sequence&);

  bool add(const Node& node);

  unsigned nnodes() const;
  const Node* node(unsigned n) const;
  const char* partition() const;
  unsigned    partitionid() const;
  
private:
  static const unsigned MaxNodes=128;
  static const unsigned MaxName=64;
  char _partition[MaxName];
  unsigned _partitionid;
  unsigned _nnodes;
  Node _nodes[MaxNodes];
};

class Kill : public Transition {
public:
  Kill(const Node& allocator);

  const Node& allocator() const;
  
private:
  Node _allocator;
};

}

#endif
