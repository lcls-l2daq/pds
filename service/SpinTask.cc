
#include "SpinTask.hh"

#include <boost/lockfree/spsc_queue.hpp>

namespace Pds {
  class spsc_queue : public boost::lockfree::spsc_queue<Routine*,boost::lockfree::capacity<32> > {
  public:
    spsc_queue() {}
  };
};

using namespace Pds;


/*
 *
 *
 *
 */
SpinTask::SpinTask(const TaskObject& tobj) :
  _taskObj (new TaskObject(tobj)),
  _refCount(new int(1)),
  _jobs    (new spsc_queue)
{
  _taskObj = new TaskObject(tobj);
  _refCount = new int(1);

  int err = createTask(*_taskObj, (TaskFunction) TaskSpinLoop );
  if ( err != 0 ) {
    //error occured, throw exception
  }
}


/*
 *
 */
SpinTask::SpinTask(const SpinTask& aTask)
{
  _refCount = aTask._refCount; ++*_refCount;
  _taskObj = aTask._taskObj;
  _jobs = aTask._jobs;
}

/*
 *
 */
void SpinTask::operator =(const SpinTask& aTask)
{
  _refCount = aTask._refCount;
  _taskObj = aTask._taskObj;
  _jobs = aTask._jobs;
}

/*
 *
 */
SpinTask::~SpinTask()
{
  // deletes only the memory for the object itself
  // memory for the contained by pointer objects is deleted 
  // by the TaskDelete object's routine()  
}


/*
 *
 *
 *
 */
void SpinTask::destroy()
{
  --*_refCount;
  if (*_refCount > 0) {
    delete this;
  }
  if (*_refCount == 0) {
    _destroyRoutine = new SpinTaskDelete(this);
    call(_destroyRoutine);
  }
  else {
    // severe error, probably bug check (assert)
  }
}




/*
 *
 *
 *
 */
const TaskObject& SpinTask::parameters() const
{
  return *_taskObj;
};


/*
 * Inserts an entry on the tasks processing queue.
 * give the jobs pending semaphore only when the queue goes empty to non-empty
 *
 */
void SpinTask::call(Routine* routine)
{
  while( !_jobs->push(routine) )
    ;
}




/*
 * Main Loop of a task.
 *
 * It is a global function with c linkage so it can be correctly
 * called by the underlying system service layer. It is friend 
 * to the Task class so that it can act like a private member 
 * function. This function should never be called directly.
 *
 * Process jobs while there are entries on the queue then take 
 * the jobs pending semaphore and wait for new entries.
 *
 */
void* Pds::TaskSpinLoop(void* task)
{
  SpinTask* t = (SpinTask*) task;
  Routine *aJob;

  for(;;) {
    if (t->_jobs->pop(aJob))
      aJob->routine();
  }

  return NULL;
}


void SpinTaskDelete::routine(void) 
{ 
  _taskToKill->deleteTask(); 
}

#include <signal.h>
#include <sched.h>

/*
 * actually make the call to start the thread
 *
 * see TaskObjectUx.cc for a note on the priority value and
 * the stupid sentinal trick.
 */
int SpinTask::createTask(TaskObject& tobj, TaskFunction aRoutine)
{
  struct sched_param param;
  param.sched_priority=tobj.priority();
  pthread_attr_setstacksize(&tobj._flags,tobj.stackSize()); 
  pthread_attr_setschedparam(&tobj._flags,&param);
 
  int status = pthread_create(&tobj._threadID,&tobj._flags,aRoutine,this);

  //  printf("Task::createTask id %d name %s\n", tobj._threadID, tobj._name);

  return status;
}


/*
 * actually make the call to exit the thread. This routine can only 
 * be called from within the context of the thread itself. Therefore,
 * only the destroy() method of Task will really use it.
 */
void SpinTask::deleteTask()
{
  //  printf("Task::deleteTask id %d\n", _taskObj->_threadID);

  if(*_refCount != 0) {
    // error as this should be the last message 
    // from the last task object for this task
  }
  delete _refCount;
  delete _taskObj;
  delete _destroyRoutine;

  delete this;

  int status;
  pthread_exit((void*)&status);
}


void SpinTask::signal(int signal){
  pthread_kill(_taskObj->_threadID, signal);
}

bool SpinTask::is_self() const
{
  return pthread_self() == (pthread_t)_taskObj->taskID();
}
