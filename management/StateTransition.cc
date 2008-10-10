#include "StateTransition.hh"

Pds::State::Id 
Pds::StateTransition::state(TransitionId::Value id)
{
  switch (id) {
  case TransitionId::Reset:
  case TransitionId::Unmap:
    return State::Standby;

  case TransitionId::Map:
  case TransitionId::Unconfigure:
    return State::Partitioned;

  case TransitionId::Configure:
  case TransitionId::EndRun:
    return State::Configured;

  case TransitionId::BeginRun:
  case TransitionId::Disable:
    return State::Ready;

  case TransitionId::Enable:
  case TransitionId::Resume:    
    return State::Enabled;

  case TransitionId::Pause:
    return State::Paused;
    
  default:
    return State::Unknown;
  }
}

Pds::TransitionId::Value 
Pds::StateTransition::transition(State::Id current, State::Id target)
{
  TransitionId::Value next;
  if (current == target) {
    // Nothing to do
    next = TransitionId::Unknown;
  } else if (current < target) {
    // Down
    next = TransitionId::Value(2*current);
  } else { 
    // Up
    next = TransitionId::Value(2*current-1);
  }
  return next;
}
