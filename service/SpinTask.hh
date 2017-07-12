#ifndef PDS_SPINTASK_H
#define PDS_SPINTASK_H

#include "TaskObject.hh"
#include "Routine.hh"

namespace Pds {

extern "C" {
  typedef void* (*TaskFunction)(void*);
  void* TaskSpinLoop(void*);
}

  class spsc_queue;

/*
 * an Task specific class that is used privately by Task so that
 * it can properly delete itself.
 *
 */

class SpinTask;
class SpinTaskDelete : public Routine {
 public:
  SpinTaskDelete(SpinTask* t) { _taskToKill = t; }
  void routine(void);
 private:
  SpinTask* _taskToKill;
};

/*
 * The Task callable interface. Customization of the code 
 *
 *
 */

class SpinTask {
 public:

  SpinTask(const TaskObject&);

  const TaskObject& parameters() const;
  void call(Routine*);
  void destroy();
  void signal(int signal);

  bool is_self() const;

  friend class SpinTaskDelete;
  friend void* TaskSpinLoop(void*);

 private:
  SpinTask() {}
  ~SpinTask();
  int createTask( TaskObject&, TaskFunction );
  void deleteTask();
  SpinTask(const SpinTask&);
  void operator=(const SpinTask&);

  TaskObject*        _taskObj;
  int*               _refCount;
  spsc_queue*        _jobs;
  Routine*           _destroyRoutine;
};
}

#endif
