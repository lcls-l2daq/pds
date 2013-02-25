#ifndef Pds_PartitionControl_hh
#define Pds_PartitionControl_hh

#include "pds/management/ControlLevel.hh"
#include "pds/utility/ControlEb.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class Arp;
  class Node;
  class ControlCallback;
  class PlatformCallback;
  class Task;
  class RunAllocator;
  class Sequencer;

  class PartitionControl : public ControlLevel {
  public:
    enum State { Unmapped, Mapped, Configured, Running, Disabled, Enabled, NumberOfStates };
  public:
    PartitionControl       (unsigned platform,
          ControlCallback&,
          int      slowReadout,
          Routine* tmo = 0,
          Arp*     arp = 0
          );
    ~PartitionControl      ();
  public:
    void  platform_rollcall(PlatformCallback*);
  public:
    bool  set_partition    (const char* name,
          const char* db_path,
          const Node* nodes,
          unsigned    nnodes,
                            uint64_t    bld_mask);
    const Allocation& partition() const;
  public:
    virtual void  set_target_state (State);
  public:
    State target_state     ()             const;
    State current_state    ()             const;
    static const char* name(State);
  public:
    unsigned get_transition_env (TransitionId::Value) const;

    //  This call blocks until Configure is complete.
    //  It will deadlock if called in the eventbuilder thread.
    void  reconfigure      (bool wait=true);
    void  pause            ();
    void  set_transition_env    (TransitionId::Value, unsigned);
    void  set_transition_payload(TransitionId::Value, Xtc*, void*);
    void  set_run          (unsigned run);
    void  set_runAllocator (RunAllocator* ra);
    void  set_experiment   (unsigned experiment);
    void  set_sequencer    (Sequencer* seq);
    void  use_run_info(bool);
  public: // Implements ControlLevel
    void  message          (const Node& hdr,
          const Message& msg);
  private:
    void  _next            ();
    void  _queue           (TransitionId::Value id);
    void  _queue           (const Transition&   tr);
    void  _complete        (TransitionId::Value id);
  public:
    void  _execute         (Transition& tr);
  public:
    const ControlEb& eb() const;
  private:
    volatile State _current_state;
    volatile State _target_state;
    volatile State _queued_target;
    Allocation _partition;
    ControlEb  _eb;
    Task*      _sequenceTask;
    Semaphore  _sem;
    unsigned   _transition_env    [TransitionId::NumberOf];
    Xtc*       _transition_xtc    [TransitionId::NumberOf];
    void*      _transition_payload[TransitionId::NumberOf];
    ControlCallback*  _control_cb;
    PlatformCallback* _platform_cb;
    RunAllocator*     _runAllocator;
    Sequencer*        _sequencer;
    unsigned   _run;
    unsigned   _experiment;
    bool       _use_run_info;
    unsigned   _pulse_id;
    Task*      _reportTask;
    friend class ControlAction;

    friend class TimeoutRecovery;
    void _eb_tmo_recovery();
    Routine*   _tmo;

    Semaphore  _sem_target;
    bool       _wait_for_target;
  };

};

#endif
