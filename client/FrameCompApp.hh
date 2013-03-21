#ifndef Pds_FrameCompApp_hh
#define Pds_FrameCompApp_hh

#include "pds/utility/Appliance.hh"

#include <list>
#include <vector>

namespace Pds {
  class Task;
  namespace FCA { class Entry; class Task; }

  class FrameCompApp : public Appliance {
  public:
    FrameCompApp(size_t max_size);
    ~FrameCompApp();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  public:
    Task& mgr_task() { return *_mgr_task; }
    void  queueTransition(Transition*);
    void  queueEvent     (InDatagram*);
    void  completeEntry  (FCA::Entry*,unsigned);
  private:
    void  _process();
  private:
    Pds::Task*              _mgr_task;
    std::list<FCA::Entry*>  _list;
    std::vector<FCA::Task*> _tasks;
  };
};

#endif
