#include "Transition.hh"

using namespace Pds;

Transition::Transition(const Transition& tr) :
  Message  (tr),
  _id      (tr._id),
  _phase   (tr._phase),
  _sequence(tr._sequence),
  _env     (tr._env)
{}

Transition::Transition(Id              id,
		       Phase           phase,
		       const Sequence& sequence,
		       unsigned        env) :
  Message(Message::Transition, sizeof(Transition)),
  _id      (id),
  _phase   (phase),
  _sequence(sequence),
  _env     (env)
{}

Transition::Id Transition::id() const {return _id;}

Transition::Phase Transition::phase() const {return _phase;}

const Sequence& Transition::sequence() const { return _sequence; }

unsigned Transition::env() const {return _env;}
