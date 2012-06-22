#ifndef PDSEVGRMANAGER_HH
#define PDSEVGRMANAGER_HH

namespace Pds {

  class Fsm;
  class Appliance;
  class Evr;
  class Evg;
  class EvgrPulseParams;
  template <class T> class EvgrBoardInfo;

  class EvgrManager {
  public:
    EvgrManager(EvgrBoardInfo<Evg>& egInfo, EvgrBoardInfo<Evr>& erInfo, unsigned npulses, EvgrPulseParams* pulse);
    Appliance& appliance();

  public:
    unsigned opcodecount() const;
    unsigned lastopcode() const;

  private:
    Evg& _eg;
    Evr& _er;
    Fsm& _fsm;
    unsigned _nplss;
    EvgrPulseParams* _pls;
  };
}

#endif
