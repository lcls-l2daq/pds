#ifndef Pds_WorkThreads_hh
#define Pds_WorkThreads_hh

#include "pds/utility/Appliance.hh"

#include <vector>

namespace Pds {
  class Task;
  namespace Work { class Manager; };

  class WorkThreads : public Appliance {
  public:
    WorkThreads(const char* name,
                const std::vector<Appliance*>&);
    ~WorkThreads();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  private:
    Task*                     _mgr_task;
    Work::Manager*            _mgr;
  };
};
    
#endif
