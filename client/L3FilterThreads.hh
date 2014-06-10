#ifndef Pds_L3FilterThreads_hh
#define Pds_L3FilterThreads_hh

#include "pds/utility/Appliance.hh"

#include "pdsdata/app/L3FilterModule.hh"

namespace Pds {
  class WorkThreads;
  class L3FilterThreads : public Appliance {
  public:
    L3FilterThreads(create_m*, unsigned nthreads=0, bool lveto=false);
    ~L3FilterThreads();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  private:
    WorkThreads* _work;
  };
};
    
#endif
