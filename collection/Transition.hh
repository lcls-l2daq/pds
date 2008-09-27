#ifndef PDSTRANSITION_HH
#define PDSTRANSITION_HH

#include "Message.hh"
#include "pds/xtc/Sequence.hh"
#include "pds/service/Pool.hh"

namespace Pds {

class Transition : public Message {
public:
  enum Id {
    Map, Unmap,
    Configure, Unconfigure,
    BeginRun, EndRun,
    Enable, Disable,
    L1Accept,
    NumberOf
  };
  enum Phase { Execute, Record };
  static const char* name(Transition::Id id) {
    return ( id < NumberOf ? _name[id] : "-Invalid-" );
  }
  Transition(const Transition&);
  Transition(Id              id,
	     Phase           phase,
	     const Sequence& sequence,
	     unsigned        env);

  Id              id      () const;
  Phase           phase   () const;
  const Sequence& sequence() const;
  unsigned        env     () const;

  PoolDeclare;
private:
  static const char* _name[Transition::NumberOf];
  Id       _id;
  Phase    _phase;
  Sequence _sequence;
  unsigned _env;
};

}
#endif
