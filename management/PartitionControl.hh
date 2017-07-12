#ifndef Pds_PartitionControl_hh
#define Pds_PartitionControl_hh

#include "pds/management/ControlLevel.hh"
#include "pds/utility/ControlEb.hh"
#include "pds/config/AliasConfigType.hh"
#include "pds/config/EvrIOConfigType.hh"
#include "pds/config/PartitionConfigType.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/alias.ddl.h"

#include <pthread.h>
// #include <set>
#include <vector>

using Pds::Alias::SrcAlias;

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
                            Routine* tmo = 0);
    ~PartitionControl      ();
  public:
    void  platform_rollcall(PlatformCallback*);
  public:
    bool  set_partition    (const char* name,
                            const char* db_path,
                            const char* l3_path,
                            const Node* nodes,
                            unsigned    nnodes,
                            unsigned    masterid,
                            const BldBitMask* bld_mask,
                            const BldBitMask* bld_mask_mon,
                            unsigned    options =0,
                            float       l3_unbias=0.,
                            const       PartitionConfigType* =0,
                            const       EvrIOConfigType* iocfg=0,
                            const       AliasConfigType* alias=0);
    bool  set_partition    (const Allocation&,
                            const       PartitionConfigType*,
                            const       EvrIOConfigType* iocfg,
                            const       AliasConfigType* alias);
    bool  set_partition    (const Allocation&,
                            const       PartitionConfigType*);
    const Allocation& partition() const;
  public:
    virtual void  set_target_state (State);
  public:
    State target_state     ()             const;
    State current_state    ()             const;
    static const char* name(State);
  public: // serialization
    void wait_for_target();
    void release_target ();
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
    Xtc*       _partition_xtc;
    Xtc*       _ioconfig_xtc;
    Xtc*       _alias_xtc;
    ControlCallback*  _control_cb;
    PlatformCallback* _platform_cb;
    RunAllocator*     _runAllocator;
    Sequencer*        _sequencer;
    unsigned   _run;
    unsigned   _experiment;
    bool       _use_run_info;
    std::list<SegPayload> _payload;
    unsigned   _pulse_id;
    Task*      _reportTask;
    friend class ControlAction;

    friend class TimeoutRecovery;
    void _eb_tmo_recovery();
    Routine*   _tmo;

    pthread_mutex_t _target_mutex;
    pthread_cond_t  _target_cond;
  };

};

#endif
