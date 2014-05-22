#ifndef Pds_L3FilterModule_hh
#define Pds_L3FilterModule_hh

#include "pds/utility/Appliance.hh"
#include "pdsdata/app/L3FilterModule.hh"

namespace Pds {
  class Task;
  namespace L3F { class Manager; };

  class L3FilterThreads : public Appliance {
  public:
    L3FilterThreads(create_m*, unsigned nthreads=0, bool lveto=false);
    ~L3FilterThreads();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  private:
    Pds::Task*               _mgr_task;
    L3F::Manager*            _mgr;
  };
};
    
#endif
