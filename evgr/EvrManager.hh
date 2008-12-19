#ifndef PDSEVRMANAGER_HH
#define PDSEVRMANAGER_HH

#include "EvgrOpcode.hh"

namespace Pds {

  class Fsm;
  class Appliance;
  class Evr;
  template <class T> class EvgrBoardInfo;

  class EvrManager {
  public:
    EvrManager(EvgrBoardInfo<Evr>& erInfo, unsigned partition,
               EvgrOpcode::Opcode opcode);
    Appliance& appliance();
  private:
    Evr& _er;
    Fsm& _fsm;
  };
}

#endif
