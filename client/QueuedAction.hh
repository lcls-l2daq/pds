#ifndef QueuedAction_hh
#define QueuedAction_hh

#include "pds/service/Routine.hh"

namespace Pds {
class Action;
class Semaphore;
class InDatagram;

  class QueuedAction : public Routine {
  public:
    QueuedAction(InDatagram* in, Action& rec, Semaphore* sem = 0) :
      _in(in), _rec(rec), _sem(sem) {}
    ~QueuedAction() {}
  public:
    void routine() ;
     
  private:
    InDatagram* _in;
    Action&  _rec;
    Semaphore*  _sem;
  };
};

#endif
