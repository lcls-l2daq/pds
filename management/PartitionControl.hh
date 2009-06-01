#ifndef Pds_PartitionControl_hh
#define Pds_PartitionControl_hh

#include "pds/management/ControlLevel.hh"
#include "pds/utility/ControlEb.hh"
#include "pdsdata/xtc/TransitionId.hh"

namespace Pds {

  class Arp;
  class Node;
  class ControlCallback;
  class PlatformCallback;
  class Task;

  class PartitionControl : public ControlLevel {
  public:
    enum State { Unmapped, Mapped, Configured, Running, Enabled, Paused };
  public:
    PartitionControl       (unsigned platform,
			    ControlCallback&,
			    Arp*     arp = 0);
    ~PartitionControl      ();
  public:
    void  platform_rollcall(PlatformCallback&);
  public:
    bool  set_partition    (const char* name,
			    const char* db_path,
			    const Node* nodes,
			    unsigned    nnodes);
  public:
    void  set_target_state (State);
    void  pause            ();
    void  resume           ();
    State target_state     ()             const;
    State current_state    ()             const;
  public:
    void  set_transition_env(TransitionId::Value, unsigned);
  private: // Implements ControlLevel
    void  message          (const Node& hdr, 
			    const Message& msg);
  private:
    void  _next            ();
    void  _queue           (TransitionId::Value id);
    void  _queue           (const Transition&   tr);
    void  _complete        (TransitionId::Value id);
  public:
    void  _execute         (Transition& tr);
  private:
    State _current_state;
    State _target_state;
    State _paused_current;
    State _paused_target;
    Allocation _partition;
    ControlEb  _eb;
    Task*      _sequenceTask;
    Semaphore  _sem;
    unsigned   _transition_env[TransitionId::NumberOf];
    ControlCallback*  _control_cb;
    PlatformCallback* _platform_cb;
    friend class ControlAction;
  };

};

#endif
