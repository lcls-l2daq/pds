#include "pds/evgr/EvrTimer.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"

using namespace Pds;

EvrTimer::EvrTimer(Appliance & app):_app(app),
				    _pool(sizeof(Occurrence), 1), _task(new Task(TaskObject("donet")))
{
}

EvrTimer::~EvrTimer()
{
  _task->destroy();
}

void EvrTimer::set_duration_ms(unsigned v)
{
  _duration = v;
}
void EvrTimer::expired()
{
  _app.post(new(&_pool) Occurrence(OccurrenceId::SequencerDone));
}
Task* EvrTimer::task()
{
  return _task;
}
unsigned EvrTimer::duration() const
{
  return _duration;
}
unsigned EvrTimer::repetitive() const
{
  return 0;
}
