#ifndef PDSFSM_HH
#define PDSFSM_HH

#include "pds/utility/Appliance.hh"

namespace Pds {
  class InDatagram;
  class Action;
  class Transition;

  class Fsm : public Appliance {
  public:
    Fsm();
    virtual ~Fsm();
    Action*     callback(TransitionId::Value id, Action* action);
  
  private:
    enum State {Idle,Mapped,Configured,Begun,Enabled,NumberOf};
    Transition* transitions(Transition*);
    InDatagram* occurrences(InDatagram* in);
    InDatagram* events     (InDatagram* in);

    unsigned    _allowed(State reqState, TransitionId::Value id);
    State       _reqState(TransitionId::Value id);
    const char* _stateName();

    State    _state;
    Action*  _action[TransitionId::NumberOf];
    Action*  _defaultAction;
  };

}

#endif
