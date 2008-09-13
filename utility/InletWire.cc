#include "InletWire.hh"
#include "pds/service/Task.hh"

using namespace Pds;

InletWire::InletWire(int taskpriority, const char* taskname) :
  _task(new Task(TaskObject(taskname, taskpriority)))
{}

InletWire::~InletWire()
{
  _task->destroy();
}

Task& InletWire::task() {return *_task;}
