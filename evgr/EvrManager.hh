#ifndef PDSEVRMANAGER_HH
#define PDSEVRMANAGER_HH

namespace Pds {

  class Fsm;
  class Appliance;
  class Evr;
  template <class T> class EvgrBoardInfo;

  class EvrManager {
  public:
    EvrManager(EvgrBoardInfo<Evr>& erInfo, unsigned partition);
    Appliance& appliance();
  private:
    Evr& _er;
    Fsm& _fsm;
  };
}

#endif
