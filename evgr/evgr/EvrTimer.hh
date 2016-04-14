#ifndef Pds_EvrTimer_hh
#define Pds_EvrTimer_hh

#include "pds/service/Timer.hh"
#include "pds/service/GenericPool.hh"

namespace Pds
{
  class Appliance;

  class EvrTimer:public Timer
  {
  public:
    EvrTimer(Appliance & app);
    ~EvrTimer();
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
    Task *                _task;
    unsigned              _duration;
  };
};

#endif
