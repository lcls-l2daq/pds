#ifndef PDSACTION_HH
#define PDSACTION_HH

namespace Pds {
  class Transition;
  class InDatagram;

  class Action {
  public:
    virtual ~Action() {}
    virtual Transition* fire(Transition* tr) { return tr; }
    virtual InDatagram* fire(InDatagram* dg) { return dg; }
  };
}

#endif
