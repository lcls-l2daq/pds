#ifndef Pds_EvrSync_hh
#define Pds_EvrSync_hh

#include "pds/service/Ins.hh"
#include "evgr/evr/evr.hh"

namespace Pds {
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
		  Client&         outlet);
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
    unsigned _target;
  public:
    static const int EVENT_CODE_SYNC = 40;
  };

  class EvrSyncSlave {
  public:
    EvrSyncSlave(EvrFIFOHandler& fifo_handler,
		 Evr&            er,
		 unsigned        partition, 
		 Task*           task);
    ~EvrSyncSlave();
  public:
    void initialize(unsigned, bool);
    bool handle(const FIFOEvent&);
    void enable();
  private:
    EvrFIFOHandler& _fifo_handler;
    Evr&     _er;
    EvrSyncState    _state;
    unsigned _target;
    Task&    _task;
    Routine* _routine;
  };
};

#endif
