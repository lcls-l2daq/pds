#ifndef PDS_INLET_HH
#define PDS_INLET_HH

#include "Appliance.hh"

namespace Pds {

class InDatagram;

class Inlet : public Appliance {

public:

  // virtual functions from Appliance

  Transition* transitions(Transition* tr) { return tr; }
  InDatagram* events     (InDatagram* dg) {return dg;}
  InDatagram* markers    (InDatagram* dg) {return dg;}
  InDatagram* occurrences(InDatagram* dg) {return dg;}
  
};
}
#endif
