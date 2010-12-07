#ifndef PDSFSM_HH
#define PDSFSM_HH

#include "pds/utility/Appliance.hh"

namespace Pds {
  class Action;
  class Response;
  class InDatagram;
  class Transition;
  class Occurrence;

  class Fsm : public Appliance {
  public:
    Fsm();
    virtual ~Fsm();
    Action*     callback(TransitionId::Value id, Action* action);
    Response*   callback(OccurrenceId::Value id, Response* response);
  
  private:
    enum State {Idle,Mapped,Configured,Begun,Disabled,Enabled,NumberOf};
    Transition* transitions(Transition*);
    Occurrence* occurrences(Occurrence*);
    InDatagram* events     (InDatagram*);

    unsigned    _allowed(State reqState, TransitionId::Value id);
    State       _reqState(TransitionId::Value id);
    const char* _stateName();

    State     _state;
    Action*   _action  [TransitionId::NumberOf];
    Response* _response[OccurrenceId::NumberOf];
    Action*   _defaultAction;
    Response* _defaultResponse;
  };

}

#endif
