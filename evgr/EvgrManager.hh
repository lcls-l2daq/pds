#ifndef PDSEVGRMANAGER_HH
#define PDSEVGRMANAGER_HH

namespace Pds {

  class Fsm;
  class Appliance;
  class Evr;
  class Evg;
  template <class T> class EvgrBoardInfo;

  class EvgrManager {
  public:
    EvgrManager(EvgrBoardInfo<Evg>& egInfo, EvgrBoardInfo<Evr>& erInfo);
    Appliance& appliance();
  private:
    Evg& _eg;
    Evr& _er;
    Fsm& _fsm;
  };
}

#endif
