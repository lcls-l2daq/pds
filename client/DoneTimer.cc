#include "pds/client/DoneTimer.hh"

#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

using namespace Pds;

DoneTimer::DoneTimer(Appliance & app):_app(app),
                                      _pool(sizeof(Occurrence), 1), 
                                      _task(new Task(TaskObject("donet")))
{
}

DoneTimer::~DoneTimer()
{
  _task->destroy();
}

void DoneTimer::set_duration_ms(unsigned v)
{
  _duration = v;
}

void DoneTimer::expired()
{
  _app.post(new(&_pool) Occurrence(OccurrenceId::SequencerDone));
}

Task* DoneTimer::task()
{
  return _task;
}

unsigned DoneTimer::duration() const
{
  return _duration;
}

unsigned DoneTimer::repetitive() const
{
  return 0;
}
