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
    Action*     callback(Transition::Id id, Action* action);
  
  private:
    enum State {Idle,Mapped,Configured,Begun,Enabled};
    Transition* transitions(Transition*);
    InDatagram* occurrences(InDatagram* in);
    InDatagram* events     (InDatagram* in);

    State    _reqState(Transition::Id id);
    unsigned _allowed(State reqState);
    State    _state;
    Action*  _action[Transition::NumberOf];
    Action*  _defaultAction;
  };

}

#endif
