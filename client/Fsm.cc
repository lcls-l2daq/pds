
#include "Fsm.hh"
#include "Action.hh"

#include <stdlib.h>

using namespace Pds;

Fsm::Fsm() : _state(Idle), _defaultAction(new Action) {
  unsigned i;
  for (i=0;i<Transition::NumberOf;i++) {
    _action[i] = _defaultAction;
  }
}

Fsm::~Fsm() {
  delete _defaultAction;
}

Fsm::State Fsm::_reqState(Transition::Id id) {
  switch (id) {
  case Transition::Map: {
    return Mapped;
    break;
  }
  case Transition::Configure: {
    return Configured;
    break;
  }
  case Transition::BeginRun: {
    return Begun;
    break;
  }
  case Transition::Enable: {
    return Enabled;
    break;
  }
  case Transition::Unmap: {
    return Idle;
    break;
  }
  case Transition::Unconfigure: {
    return Mapped;
    break;
  }
  case Transition::EndRun: {
    return Configured;
    break;
  }
  case Transition::Disable: {
    return Begun;
    break;
  }
  case Transition::L1Accept: {
    return Enabled;
    break;
  }
  default: {
    printf("Request for illegal transition %s\n",Transition::name(id));
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
  Transition::Id id = tr->id();
  printf("Received id %d\n",id);
  State reqState = _reqState(id);
  if (_allowed(reqState)) {
    _action[id]->fire(tr);
    // need to check for failure of the action before updating _state
    _state = reqState;
  } else {
    // assert invalid transition damage here
    printf("Invalid transition %s\n",Transition::name(id));
  }
  return tr;
}

Action* Fsm::callback(Transition::Id id, Action* action) {
  Action* oldAction = _action[id];
  _action[id]=action;
  return oldAction;
}
