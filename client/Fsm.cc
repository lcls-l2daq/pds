
#include "Fsm.hh"
#include "Action.hh"

#include <stdlib.h>

using namespace Pds;

Fsm::Fsm() : _state(Idle), _defaultAction(new Action) {
  unsigned i;
  for (i=0;i<TransitionId::NumberOf;i++) {
    _action[i] = _defaultAction;
  }
}

Fsm::~Fsm() {
  delete _defaultAction;
}

Fsm::State Fsm::_reqState(TransitionId::Value id) {
  switch (id) {
  case TransitionId::Map: {
    return Mapped;
    break;
  }
  case TransitionId::Configure: {
    return Configured;
    break;
  }
  case TransitionId::BeginRun: {
    return Begun;
    break;
  }
  case TransitionId::Enable: {
    return Enabled;
    break;
  }
  case TransitionId::Unmap: {
    return Idle;
    break;
  }
  case TransitionId::Unconfigure: {
    return Mapped;
    break;
  }
  case TransitionId::EndRun: {
    return Configured;
    break;
  }
  case TransitionId::Disable: {
    return Begun;
    break;
  }
  case TransitionId::L1Accept: {
    return Enabled;
    break;
  }
  default: {
    printf("Request for illegal transition %s\n",TransitionId::name(id));
    return _state;
    break;
  }
  }
}

unsigned Fsm::_allowed(State reqState) {
  return (abs((unsigned)reqState-_state)==1);
}

InDatagram* Fsm::events(InDatagram* in) {
  return in;
}

InDatagram* Fsm::occurrences(InDatagram* in) {
  return in;
}

Transition* Fsm::transitions(Transition* tr) {
  TransitionId::Value id = tr->id();
  State reqState = _reqState(id);
  if (_allowed(reqState)) {
    _action[id]->fire(tr);
    // need to check for failure of the action before updating _state
    _state = reqState;
  } else {
    // assert invalid transition damage here
    printf("Invalid transition %s\n",TransitionId::name(id));
  }
  return tr;
}

Action* Fsm::callback(TransitionId::Value id, Action* action) {
  Action* oldAction = _action[id];
  _action[id]=action;
  return oldAction;
}
