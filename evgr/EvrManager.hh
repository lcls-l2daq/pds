#ifndef PDSEVRMANAGER_HH
#define PDSEVRMANAGER_HH

#include "EvgrOpcode.hh"

namespace Pds {

  class CfgClientNfs;
  class Fsm;
  class Appliance;
  class Evr;
  class DoneTimer;
  template <class T> class EvgrBoardInfo;

  class EvrManager {
  public:
    EvrManager(EvgrBoardInfo<Evr>& erInfo, 
	       CfgClientNfs& cfg);
    ~EvrManager();
  public:
    Appliance& appliance();
  public:
    // for testing
    static void drop_pulses(unsigned mask);
  private:
    Evr& _er;
    Fsm& _fsm;
    DoneTimer* _done;
  };
}

#endif
