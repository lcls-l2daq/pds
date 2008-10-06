#ifndef STATETRANSITION_HH
#define STATETRANSITION_HH

#include "pds/utility/State.hh"
#include "pds/utility/Transition.hh"

namespace Pds {
  namespace StateTransition {
    State::Id state(Transition::Id id);
    Transition::Id transition(State::Id current, State::Id target);
  };
}

#endif
