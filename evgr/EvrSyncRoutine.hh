#ifndef Pds_EvrSyncRoutine_hh
#define Pds_EvrSyncRoutine_hh

#include "pds/service/Routine.hh"

#include "pds/service/Ins.hh"
#include "pds/service/NetServer.hh"
#include "pds/service/OobPipe.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {
  class EvrSyncCallback;

  class EvrSyncRoutine : public Routine {
  public:
    EvrSyncRoutine(unsigned         partition,
                   EvrSyncCallback& sync);
    virtual ~EvrSyncRoutine();
  public:
    void routine();
  private:
    Ins              _group;
    NetServer        _server;
    OobPipe          _loopback;
    EvrSyncCallback& _sync;
    Semaphore        _sem;
  };
};

#endif
