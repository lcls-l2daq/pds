#ifndef PDSTRANSITION_HH
#define PDSTRANSITION_HH

#include "Message.hh"

namespace Pds {

class Transition : public Message {
public:
  enum Id {
    Map, Unmap,
    Configure, Unconfigure,
    BeginRun, EndRun,
    Pause, Resume,
    Enable, Disable,
    SoftL1
  };
  Transition(Id id);

  Id id() const;

private:
  Id _id;
};

  class L1Transition : public Transition {
  public:
    L1Transition(unsigned key) : Transition(Transition::SoftL1), _key(key) 
    {
      _size = sizeof(*this);
    }
    
    unsigned key() const { return _key; }
    
  private:
    unsigned _key;
  };

}
#endif
