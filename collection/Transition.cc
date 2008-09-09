#include "Transition.hh"

using namespace Pds;

Transition::Transition(Id id) :
  Message(Message::Transition, sizeof(Transition)),
  _id(id)
{}

Transition::Id Transition::id() const {return _id;}
