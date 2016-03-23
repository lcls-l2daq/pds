#ifndef Pds_EvrSync_hh
#define Pds_EvrSync_hh

#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Ins.hh"
#include "evgr/evr/evr.hh"

namespace Pds {
  class Appliance;
  class Task;
  class Routine;
  class Client;
  class EvrFIFOHandler;

  enum EvrSyncState { EnableInit, EnableSeek, Enabled, DisableInit, DisableSeek, Disabled };

  class EvrSyncMaster {
  public:
    EvrSyncMaster(EvrFIFOHandler& fifo_handler,
		  Evr&            er,
		  unsigned        partition, 
		  Task*           task,
		  Client&         outlet,
                  Appliance&      app);
    ~EvrSyncMaster();
  public:
    void initialize(bool);
    bool handle(const FIFOEvent&);
  private:
    EvrFIFOHandler& _fifo_handler;
    Evr&     _er;
    EvrSyncState _state;
    Ins      _dst;
    Task&    _task;
    Client&  _outlet;
    Appliance& _app;
    GenericPool _pool;
    unsigned _target;
  public:
    static const int EVENT_CODE_SYNC = 40;
  };

  class EvrSyncSlave : public EvrSyncCallback {
  public:
    EvrSyncSlave(EvrFIFOHandler& fifo_handler,
		 Evr&            er,
		 unsigned        partition, 
		 Task*           task,
                 Task*           sync_task);
    ~EvrSyncSlave();
  public:
    void initialize(unsigned, bool);
  public:
    bool handle(const FIFOEvent&);
    void enable();
  private:
    EvrFIFOHandler& _fifo_handler;
    Evr&     _er;
    EvrSyncState    _state;
    unsigned _target;
    Task&    _task;
    Task&    _sync_task;
    Routine* _routine;
  };
};

#endif
