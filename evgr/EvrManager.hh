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
    EvrManager(EvgrBoardInfo<Evr>& erInfo, CfgClientNfs& cfg, bool bTurnOffBeamCodes);
    ~EvrManager();

    // SIGINT handler
    static void sigintHandler(int);
    
    Appliance& appliance();

    // for testing
    static void drop_pulses(unsigned mask);
  private:
    Evr&        _er;
    Fsm&        _fsm;
    DoneTimer*  _done;
    bool        _bTurnOffBeamCodes;
    
  public:
    //static const int EVENT_CODE_BEAM  = 40;
    //static const int EVENT_CODE_BYKIK = 41;
    static const int EVENT_CODE_BEAM  = 140;
    static const int EVENT_CODE_BYKIK = 162;
  };
}

#endif
