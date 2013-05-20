#ifndef Pds_DoneTimer_hh
#define Pds_DoneTimer_hh

#include "pds/service/Timer.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {
  class Appliance;
  class Task;

  class DoneTimer:public Timer
  {
  public:
    DoneTimer(Appliance & app);
    ~DoneTimer();
  public:
    void set_duration_ms(unsigned v);
  public:
    void expired();
    Task *task();
    unsigned duration() const;
    unsigned repetitive() const;
  private:
    Appliance &           _app;
    GenericPool           _pool;
    Task*                 _task;
    unsigned              _duration;
  };
};

#endif
