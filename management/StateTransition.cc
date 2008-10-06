#include "StateTransition.hh"

Pds::State::Id 
Pds::StateTransition::state(Transition::Id id)
{
  switch (id) {
  case Transition::Reset:
  case Transition::Unmap:
    return State::Standby;

  case Transition::Map:
  case Transition::Unconfigure:
    return State::Partitioned;

  case Transition::Configure:
  case Transition::EndRun:
    return State::Configured;

  case Transition::BeginRun:
  case Transition::Disable:
    return State::Ready;

  case Transition::Enable:
  case Transition::Resume:    
    return State::Enabled;

  case Transition::Pause:
    return State::Paused;
    
  default:
    return State::Unknown;
  }
}

Pds::Transition::Id 
Pds::StateTransition::transition(State::Id current, State::Id target)
{
  Transition::Id next;
  if (current == target) {
    // Nothing to do
    next = Transition::Unknown;
  } else if (current < target) {
    // Down
    next = Transition::Id(2*current);
  } else { 
    // Up
    next = Transition::Id(2*current-1);
  }
  return next;
}
