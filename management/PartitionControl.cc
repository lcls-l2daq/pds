#include "pds/management/PartitionControl.hh"

#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"

#include "pds/service/Semaphore.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/PlatformCallback.hh"

#include "pds/collection/PingReply.hh"
#include "pds/service/Task.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Routine.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

namespace Pds {

  class SequenceJob : public Routine {
  public:
    SequenceJob(PartitionControl& control,
                const Transition& tr) :
      _control(control)
    { memcpy(_buffer, &tr, tr.size()); }
  public:
    void routine() {
      _control._execute(*reinterpret_cast<Transition*>(_buffer));
      delete this;
    }
  private:
    PartitionControl& _control;
    char              _buffer[sizeof(Allocate)];
  };

  class ControlAction : public Appliance {
  public:
    ControlAction(PartitionControl& control) :
      _control(control)
    {}
    ~ControlAction() {}
  public:
    Transition* transitions(Transition* i) {
      if (i->phase() == Transition::Execute) {
        if (i->id()==TransitionId::Disable) {
          //
          //  The disable transition often splits the last L1Accept.
          //  There is no way to know when the last L1A has passed through
          //  all levels, so wait some reasonable time.
          //
          timespec tv;
          tv.tv_sec = 0; tv.tv_nsec = 50000000;
          nanosleep(&tv, 0);
        }
	if (i->id()==TransitionId::Unmap) {
	  _control._complete(i->id());
	}
	else {
	  Transition tr(i->id(),
			Transition::Record,
			Sequence(Sequence::Event,
				 i->id(),
				 i->sequence().clock(),
				 i->sequence().stamp()),
			i->env() );
	  _control.mcast(tr);
	}
      }
      return i;
    }
    InDatagram* events     (InDatagram* i) { _control._complete(i->datagram().seq.service()); return i; }
  private:
    PartitionControl& _control;
  };

  class MyCallback : public ControlCallback {
  public:
    MyCallback(PartitionControl& o,
	       ControlCallback& cb) : _outlet(&o), _cb(cb) {}
    void attached(SetOfStreams& streams) {
      _cb.attached(streams);
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      (new ControlAction(*_outlet))   ->connect(frmk.inlet());
    }
    void failed(Reason reason) {
      printf("Platform failed to attach: reason %d\n", reason);
    }
    void dissolved(const Node& node) {
      printf("Partition dissolved by uid %d pid %d ip %x\n",
             node.uid(), node.pid(), node.ip());
    }
  private:
    PartitionControl*  _outlet;
    ControlCallback&   _cb;
  };

};

using namespace Pds;

PartitionControl::PartitionControl(unsigned platform,
				   ControlCallback& cb,
				   Routine* tmo,
				   Arp*     arp) :
  ControlLevel    (platform, *new MyCallback(*this, cb), arp),
  _current_state  (Unmapped),
  _target_state   (Unmapped),
  _eb             (header(),tmo),
  _sequenceTask   (new Task(TaskObject("controlSeq"))),
  _sem            (Semaphore::EMPTY),
  _control_cb     (&cb),
  _platform_cb    (0)
{
  memset(_transition_env,0,TransitionId::NumberOf*sizeof(unsigned));
}

PartitionControl::~PartitionControl() 
{
  _sequenceTask->destroy(); 
}

void PartitionControl::platform_rollcall(PlatformCallback* cb)
{
  if ((_platform_cb = cb)) {
    Message ping(Message::Ping);
    mcast(ping);
  }
}

bool PartitionControl::set_partition(const char* name,
				     const char* dbpath,
				     const Node* nodes,
				     unsigned    nnodes)
{
  _partition = Allocation(name,dbpath,partitionid());
  for(unsigned k=0; k<nnodes; k++)
    _partition.add(nodes[k]);
  return true;
}

void PartitionControl::set_target_state(State state)
{
  State prev_target = _target_state;
  _target_state = state;
  if (prev_target == _current_state)
    _next();
}

PartitionControl::State PartitionControl::target_state () const { return _target_state; }
PartitionControl::State PartitionControl::current_state() const { return _current_state; }
void  PartitionControl::set_transition_env(TransitionId::Value tr, unsigned env)
{ _transition_env[tr] = env; }

void PartitionControl::message(const Node& hdr, const Message& msg)
{
  switch(msg.type()) {
  case Message::Ping:
  case Message::Join:
    if (!_isallocated && _platform_cb) _platform_cb->available(hdr,static_cast<const PingReply&>(msg));
    break;
  case Message::Transition:
    {
      const Transition& tr = reinterpret_cast<const Transition&>(msg);
      if (tr.phase() == Transition::Execute) {

	if (hdr==header()) {
	}

	Transition* out = _eb.build(hdr,tr);
	if (!out) return;

	PartitionMember::message(header(),*out);
	delete out;
	_sem.give();
      }
    }
    break;
  case Message::Occurrence:
    {
      const Occurrence& occ = reinterpret_cast<const Occurrence&>(msg);
      switch(occ.id()) {
      case OccurrenceId::ClearReadout:
	_queue(TransitionId::Disable);
	break;
      default:
	break;
      }
    }
  default:
    break;
  }
  ControlLevel::message(hdr,msg);
}

void PartitionControl::_next()
{
  //  printf("_next  current %d  target %d\n",_current_state, _target_state);

  if      (_current_state==_target_state) 
    ;
  else if (_target_state > _current_state) 
    switch(_current_state) {
    case Unmapped  : { Allocate alloc(_partition); _queue(alloc); break; }
    case Mapped    : _queue(TransitionId::Configure      ); break;
    case Configured: _queue(TransitionId::BeginRun       ); break;
    case Running   : _queue(TransitionId::BeginCalibCycle); break;
    case Disabled  : _queue(TransitionId::Enable         ); break;
    default: break;
    }
  else if (_target_state < _current_state) 
    switch(_current_state) {
    case Mapped    : { Kill kill(header()); _queue(kill); break; }
    case Configured: _queue(TransitionId::Unconfigure  ); break;
    case Running   : _queue(TransitionId::EndRun       ); break;
    case Disabled  : _queue(TransitionId::EndCalibCycle); break;
    case Enabled   : _queue(TransitionId::Disable      ); break;
    default: break;
    }
}

void PartitionControl::_complete(TransitionId::Value id)
{
  switch(id) {
  case TransitionId::Unmap          : _current_state = Unmapped  ; break;
  case TransitionId::Map            :
  case TransitionId::Unconfigure    : _current_state = Mapped    ; break;
  case TransitionId::Configure      :
  case TransitionId::EndRun         : _current_state = Configured; break;
  case TransitionId::BeginRun       :
  case TransitionId::EndCalibCycle  : _current_state = Running   ; break;
  case TransitionId::BeginCalibCycle:
  case TransitionId::Disable        : _current_state = Disabled  ; break;
  case TransitionId::Enable         : _current_state = Enabled   ; break;
  default: break;
  }
  _next();
}

void PartitionControl::_queue(TransitionId::Value id)
{
  //  timestamp and pulseId should come from master EVR
  //  fake it for now
  Transition tr(id, _transition_env[id]);
  _queue(tr);
}

void PartitionControl::_queue(const Transition& tr) {
  _sequenceTask->call( new SequenceJob(*this,tr) );
}

void PartitionControl::_execute(Transition& tr) {

  _eb.reset(_partition);

  mcast(tr);

  _sem.take();  // block until transition is complete
}

const ControlEb& PartitionControl::eb() const { return _eb; }
