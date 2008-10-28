#ifndef PDS_DMAENGINE
#define PDS_DMAENGINE

#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"

namespace Pds {

  class DmaEngine : public Routine {
  public:
    DmaEngine(Task* task) : _task(task) {}
    void start(void* destination) {
      _destination=destination;
      _task->call(this);
    }
    virtual void routine(void) = 0;
  protected:
    Task* _task;
    void* _destination;
  };

}

#endif
