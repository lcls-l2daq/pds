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
    Pause, Resume,
    Enable, Disable,
    L1Accept
  };
  enum Phase { Execute, Record };

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
  Id       _id;
  Phase    _phase;
  Sequence _sequence;
  unsigned _env;
};
}
#endif
