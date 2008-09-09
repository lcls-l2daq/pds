#include "SelectDriver.hh"
#include "Select.hh"
#include "Task.hh"

using namespace Pds;

SelectDriver::SelectDriver(Select& select) :
  _select(select)
{}

void SelectDriver::run(Task& task) 
{
  task.call(this);
}

void SelectDriver::routine()
{
  while (_select.poll());
}
