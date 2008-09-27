#ifndef PDSACTION_HH
#define PDSACTION_HH

namespace Pds {
  class Transition;

  class Action {
  public:
    virtual ~Action() {}
    virtual void fire(Transition*) {}
  };
}

#endif
