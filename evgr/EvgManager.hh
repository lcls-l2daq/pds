#ifndef PDSEVGRMANAGER_HH
#define PDSEVGRMANAGER_HH

namespace Pds {

  class Fsm;
  class Appliance;
  class Evg;
  class EvgMasterTiming;
  class EvgrPulseParams;
  template <class T> class EvgrBoardInfo;

  class EvgManager {
  public:
    EvgManager(EvgrBoardInfo<Evg>& egInfo, 
                const EvgMasterTiming&  mtime);
    Appliance& appliance();

  private:
    Evg& _eg;
    Fsm& _fsm;
  };
}

#endif
