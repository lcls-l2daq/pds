#include "pds/management/PartitionControl.hh"

#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"

#include "pds/service/Semaphore.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/RunAllocator.hh"
#include "pds/management/Sequencer.hh"

#include "pds/collection/PingReply.hh"
#include "pds/service/Task.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"

#include "pdsdata/xtc/Sequence.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

static const unsigned MaxPayload = 0x1000;

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

  class ReportJob : public Routine {
  public:
    ReportJob(RunAllocator& allocator,
	      int expt, int run, int stream, int chunk) :
      _allocator(allocator), 
      _expt(expt), _run(run), _stream(stream), _chunk(chunk) {}
  public:
    void routine() { 
      _allocator.reportOpenFile(_expt,_run,_stream,_chunk); 
      delete this;
    }
  private:
    RunAllocator& _allocator;
    int _expt, _run, _stream, _chunk;
  };

  class ControlAction : public Appliance {
  public:
    ControlAction(PartitionControl& control) :
      _control(control),
      _pool   (sizeof(Transition)+MaxPayload,1)
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
          tv.tv_sec = 0; tv.tv_nsec = 200000000;
	  nanosleep(&tv, 0);
	}
	else if (i->id()==TransitionId::Map) {
          //
	  //  The map transition instructs the event-builder streams 
	  //  to register for multicasts.  Without some ping/reply
	  //  protocol, we can't verify the registration is complete;
	  //  so wait some reasonable time.
          //
          timespec tv;
          tv.tv_sec = 0; tv.tv_nsec = 50000000;
          nanosleep(&tv, 0);
        }
	if (i->id()==TransitionId::Unmap) {
	  _control._complete(i->id());
	}
	else {
	  Transition* tr;
	  const Xtc* xtc = _control._transition_xtc[i->id()];
	  if (xtc && xtc->extent < MaxPayload) {
	    tr = new(&_pool) Transition(i->id(),
					Transition::Record,
					Sequence(Sequence::Event,
						 i->id(),
						 i->sequence().clock(),
						 i->sequence().stamp()),
					i->env(),
					sizeof(Transition)+xtc->extent);
	    char* p = reinterpret_cast<char*>(tr+1);
	    memcpy(p,xtc,sizeof(Xtc));
	    memcpy(p += sizeof(Xtc),_control._transition_payload[i->id()],xtc->sizeofPayload());
	  }
	  else {
	    if (xtc) 
	      printf("PartitionControl transition payload size (0x%x) exceeds maximum.  Discarding.\n",xtc->extent);

	    tr = new(&_pool) Transition(i->id(),
					Transition::Record,
					Sequence(Sequence::Event,
						 i->id(),
						 i->sequence().clock(),
						 i->sequence().stamp()),
					i->env());
	  }
	  _control.mcast(*tr);
	  delete tr;
	}
      }
      return i;
    }
    InDatagram* events     (InDatagram* i) {
//       if (i->datagram().xtc.damage.value()&(1<<Damage::UserDefined))
// 	_control.set_target_state(_control.current_state());  // backout one step

      _control._complete(i->datagram().seq.service()); 
      return i; 
    }
  private:
    PartitionControl& _control;
    GenericPool _pool;
  };

  class MyCallback : public ControlCallback {
  public:
    MyCallback(PartitionControl& o,
	       ControlCallback& cb) : _outlet(&o), _cb(cb) {}
    void attached(SetOfStreams& streams) {
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      (new ControlAction(*_outlet))   ->connect(frmk.inlet());
      _cb.attached(streams);
    }
    void failed(Reason reason) {
      printf("Platform failed to attach: reason %d\n", reason);
      _cb.failed(reason);
    }
    void dissolved(const Node& node) {
      printf("Partition dissolved by uid %d pid %d ip %x\n",
             node.uid(), node.pid(), node.ip());
      _cb.dissolved(node);
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
  _queued_target  (Mapped),
  _eb             (header(),tmo),
  _sequenceTask   (new Task(TaskObject("controlSeq"))),
  _sem            (Semaphore::EMPTY),
  _control_cb     (&cb),
  _platform_cb    (0),
  _sequencer      (0),
  _experiment     (0),
  _use_run_info   (true),
  _reportTask     (new Task(TaskObject("controlRep")))
{
  memset(_transition_env,0,TransitionId::NumberOf*sizeof(unsigned));
  memset(_transition_xtc,0,TransitionId::NumberOf*sizeof(Xtc*));
}

PartitionControl::~PartitionControl() 
{
  _sequenceTask->destroy(); 
  _reportTask  ->destroy(); 
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

const Allocation& PartitionControl::partition() const
{ return _partition; }

void PartitionControl::set_target_state(State state)
{
  printf("PartitionControl::set_target_state curr %s  prevtgt %s  tgt %s\n",
	 name(_current_state), name(_target_state), name(state));

  State prev_target = _target_state;
  _target_state = state;
  if (prev_target == _current_state)
    _next();
}

PartitionControl::State PartitionControl::target_state () const { return _target_state; }
PartitionControl::State PartitionControl::current_state() const { return _current_state; }

void PartitionControl::reconfigure()
{
  if (_target_state > Mapped) {
    _queued_target = _target_state;
    set_target_state(Mapped);
  }
}

void PartitionControl::pause()
{
  if (_target_state > Disabled) {
    _queued_target = _target_state;
    set_target_state(Disabled);
  }
}

void  PartitionControl::set_runAllocator (RunAllocator* ra) {
  _runAllocator = ra;
}

void  PartitionControl::set_experiment   (unsigned experiment) {
  _experiment=experiment;
}

void  PartitionControl::set_sequencer    (Sequencer* seq) {
  _sequencer = seq;
}

void  PartitionControl::use_run_info(bool r) {
  _use_run_info=r;
}

unsigned PartitionControl::get_transition_env(TransitionId::Value tr) const
{ return _transition_env[tr]; }

void  PartitionControl::set_transition_env(TransitionId::Value tr, unsigned env)
{ _transition_env[tr] = env; }
void  PartitionControl::set_transition_payload(TransitionId::Value tr, Xtc* xtc, void* payload)
{
  _transition_xtc    [tr] = xtc; 
  _transition_payload[tr] = payload; 
}

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
	printf("Received ClearReadout occurrence from %x/%d\n",
	       hdr.procInfo().ipAddr(),
	       hdr.procInfo().processId());
	//	reconfigure();
	break;
      case OccurrenceId::DataFileOpened: {
	const DataFileOpened& dfo = reinterpret_cast<const DataFileOpened&>(occ);
	printf("Received DataFileOpened occurrence from %x/%d [r%04d-s%02d-c%02d]\n",
	       hdr.procInfo().ipAddr(),
	       hdr.procInfo().processId(),
	       dfo.run, dfo.stream, dfo.chunk);
	if (_runAllocator)
	  _reportTask->call(new ReportJob(*_runAllocator, dfo.expt, dfo.run, dfo.stream, dfo.chunk));
	break;
      }
      case OccurrenceId::RequestPause:
	printf("Received RequestPause occurrence from %x/%d\n",
	       hdr.procInfo().ipAddr(),
	       hdr.procInfo().processId());
        pause();
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
  if      (_current_state==_target_state) 
    ;
  else if (_target_state > _current_state) 
    switch(_current_state) {
    case Unmapped  : { Allocate alloc(_partition); _queue(alloc); break; }
    case Mapped    : _queue(TransitionId::Configure      ); break;
    case Configured: {
      if (_use_run_info) {
	unsigned run = _runAllocator->alloc();
	if (run!=RunAllocator::Error) {
	  RunInfo rinfo(run,_experiment); _queue(rinfo);
	}
      }
      else {
	timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	Transition rinfo(TransitionId::BeginRun, ts.tv_sec);
	_queue(rinfo );
      }
      break;
    }
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
    case Enabled   : 
      if (_sequencer) _sequencer->stop();
      _queue(TransitionId::Disable); 
      break;
    default: break;
    }
}

void PartitionControl::_complete(TransitionId::Value id)
{
  State current = _current_state;
  switch(id) {
  case TransitionId::Unmap          : _current_state = Unmapped  ; break;
  case TransitionId::Unconfigure    : 
    if (_target_state==Mapped) { 
      _current_state = _target_state;
      set_target_state(_queued_target);
      _queued_target=Mapped;
      return;
    }
  case TransitionId::Map            : _current_state = Mapped    ; break;
  case TransitionId::Configure      :
  case TransitionId::EndRun         : _current_state = Configured; break;
  case TransitionId::BeginRun       :
  case TransitionId::EndCalibCycle  : _current_state = Running   ; break;
  case TransitionId::BeginCalibCycle:
  case TransitionId::Disable        : 
    _current_state = Disabled;
    if (_target_state==Disabled && _queued_target>Disabled) {
      set_target_state(_queued_target);
      _queued_target=Disabled;
      return;
    }
    break;
  case TransitionId::Enable         : 
    _current_state = Enabled   ; 
    if (_sequencer) _sequencer->start();
    break;
  case TransitionId::L1Accept       : return;
  default: break;
  }
  if (current != _current_state)
    _next();
}

void PartitionControl::_queue(TransitionId::Value id)
{
  Transition tr(id, _transition_env[id], sizeof(Transition));
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

const char* PartitionControl::name(State s)
{
  static const char* names[] = {"Unmapped", "Mapped", "Configured", "Running",
				"Disabled", "Enabled", NULL };
  return names[s];
}
