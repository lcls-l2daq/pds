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
#include "pds/xtc/XtcType.hh"

#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"
#include "pds/config/AliasConfigType.hh"

#include "pdsdata/xtc/Sequence.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <string> // Required for std::string
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <list>

#define RECOVER_TMO
#define USE_L1A
#define DBUG

static const unsigned MaxPayload = 0x1800;
static timespec disable_tmo;

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
              int expt, int run, int stream, int chunk, std::string& hostname, std::string& fname) :
      _allocator(allocator),
      _expt(expt), _run(run), _stream(stream), _chunk(chunk), _host_name(hostname), _fname(fname) {}
  public:
    void routine() {
      _allocator.reportOpenFile(_expt,_run,_stream,_chunk,_host_name,_fname);
      delete this;
    }
  private:
    RunAllocator& _allocator;
    int _expt, _run, _stream, _chunk;
    std::string _host_name;
    std::string _fname;
  };

  class ReportDetectors : public Routine {
  public:
    ReportDetectors(RunAllocator& allocator,
                    int expt, int run, std::vector<std::string>& names) :
      _allocator(allocator), _expt(expt), _run(run), _names(names) {}
  public:
    void routine() {
      _allocator.reportDetectors(_expt,_run,_names);
      delete this;
    }
  private:
    RunAllocator& _allocator;
    int _expt, _run, _stream, _chunk;
    std::vector<std::string> _names;
  };

  class ControlAction : public Appliance {
  public:
    ControlAction(PartitionControl& control) :
      _control(control),
      _pool   (sizeof(Transition)+MaxPayload,1)
    {
    }
    ~ControlAction()
    {
    }
  public:
    Transition* transitions(Transition* i) {
      if (i->phase() == Transition::Execute) {
#ifdef USE_L1A
        if (i->id()==TransitionId::L1Accept) {
          //
          //  The disable transition often splits the last L1Accept.
          //  There is no way to know when the last L1A has passed through
          //  all levels, so wait some reasonable time.
          //
          timespec tv = disable_tmo;
          //          tv.tv_sec = 0; tv.tv_nsec = 200000000;
          //
          //  Cspad + Opal1k seems to need more time?
          //
//#ifdef BUILD_SLOW_DISABLE // no need if princeton runs with "delay shots"
//          tv.tv_sec = 1; tv.tv_nsec = 0;
//#else
//          tv.tv_sec = 0; tv.tv_nsec = 300000000; // 300 ms
//#endif
          //  This works!
          //          tv.tv_sec = 0; tv.tv_nsec = 300000000; // 300 ms

          //  20 milliseconds is apparently not long enough
          //  But try for XPP again
          //          tv.tv_sec = 0; tv.tv_nsec = 20000000;
          nanosleep(&tv, 0);

    const_cast<ControlEb&>(_control.eb()).reset(_control.partition());

          Transition* tr = new(&_pool) Transition(TransitionId::Disable,
                                                  Transition::Execute,
                                                  Sequence(Sequence::Event,
                                                           i->id(),
                                                           i->sequence().clock(),
                                                           i->sequence().stamp()),
                                                  i->env());
          _control.mcast(*tr);

          delete tr;
          return i;
        }
#else
        if (i->id()==TransitionId::Disable) {
          //
          //  The disable transition often splits the last L1Accept.
          //  There is no way to know when the last L1A has passed through
          //  all levels, so wait some reasonable time.
          //
          timespec tv;
          //          tv.tv_sec = 0; tv.tv_nsec = 200000000;
          //
          //  Cspad + Opal1k seems to need more time?
          //
//#ifdef BUILD_SLOW_DISABLE // no need if princeton runs with "delay shots"
//          tv.tv_sec = 2; tv.tv_nsec = 0;
//#else
//          tv.tv_sec = 0; tv.tv_nsec = 300000000; // 300 ms
//#endif
          tv.tv_sec = 0; tv.tv_nsec = 300000000; // 300 ms

          //  20 milliseconds is apparently not long enough
          //          tv.tv_sec = 0; tv.tv_nsec = 20000000;
          nanosleep(&tv, 0);
        }
#endif
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
	  bool append_alias = (i->id()==TransitionId::Configure) && (_control.count_src_alias() > 0);

	  if (xtc && append_alias) {
	    unsigned payload_size = xtc->extent + 2*sizeof(Xtc);
	    payload_size += sizeof(AliasConfigType)+_control.count_src_alias()*sizeof(Pds::SrcAlias);

	    if (payload_size > MaxPayload) {
              printf("PartitionControl transition payload size (0x%x) exceeds maximum.  Aborting.\n",payload_size);
	      abort();
	    }

            tr = new(&_pool) Transition(i->id(),
                                        Transition::Record,
                                        Sequence(Sequence::Event,
                                                 i->id(),
                                                 i->sequence().clock(),
                                                 i->sequence().stamp()),
                                        i->env(),
					sizeof(Transition)+payload_size);

	    Xtc* top = new(reinterpret_cast<char*>(tr+1)) Xtc(_xtcType,xtc->src);
	    //  Attach the control_transition header and payload
	    { 
	      Xtc* cxtc = new(reinterpret_cast<char*>(top->next())) Xtc(*xtc);
	      memcpy(cxtc->alloc(xtc->sizeofPayload()),_control._transition_payload[i->id()],xtc->sizeofPayload());
	      top->extent += cxtc->extent; 
	    }
	    //  Attach the alias configuration header and payload
	    { 
	      Xtc* alias = new(reinterpret_cast<char*>(top->next())) Xtc(_aliasConfigType,xtc->src);
	      *reinterpret_cast<uint32_t*>(alias->alloc(sizeof(uint32_t))) = _control.count_src_alias();
	      for(std::list<SrcAlias>::const_iterator it=_control._src_aliases.begin(); it!=_control._src_aliases.end(); it++)
		new(alias->alloc(sizeof(SrcAlias))) SrcAlias(*it);
	      top->extent += alias->extent; 
	    }
	  }

	  else if (xtc) { 
	    if (xtc->extent > MaxPayload) {
              printf("PartitionControl transition payload size (0x%x) exceeds maximum.  Aborting.\n",xtc->extent);
	      abort();
	    }

	    tr = new(&_pool) Transition(i->id(),
                                        Transition::Record,
                                        Sequence(Sequence::Event,
                                                 i->id(),
                                                 i->sequence().clock(),
                                                 i->sequence().stamp()),
                                        i->env(),
					sizeof(Transition)+xtc->extent);
	    Xtc* top = new(reinterpret_cast<char*>(tr+1)) Xtc(*xtc);
	    memcpy(top->alloc(xtc->sizeofPayload()),_control._transition_payload[i->id()],xtc->sizeofPayload());
	  }

	  else {
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
      //  _control.set_target_state(_control.current_state());  // backout one step

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

  class TimeoutRecovery : public Routine {
  public:
    TimeoutRecovery(PartitionControl& control) : _control(control) {}
  public:
    void routine() {
      _control._eb_tmo_recovery();
    }
  private:
    PartitionControl& _control;
  };
};

using namespace Pds;

PartitionControl::PartitionControl(unsigned platform,
                                   ControlCallback& cb,
                                   int      slowReadout,
                                   Routine* tmo,
                                   Arp*     arp
                                   ) :
  ControlLevel    (platform, *new MyCallback(*this, cb), slowReadout, arp),
  _current_state  (Unmapped),
  _target_state   (Unmapped),
  _queued_target  (Mapped),
#ifdef RECOVER_TMO
  _eb             (header(),new TimeoutRecovery(*this), (slowReadout > 0? 60000: 10000)),
#else
  _eb             (header(),tmo, (slowReadout > 0? 60000: 10000)),
#endif
  _sequenceTask   (new Task(TaskObject("controlSeq"))),
  _sem            (Semaphore::EMPTY),
  _control_cb     (&cb),
  _platform_cb    (0),
  _sequencer      (0),
  _experiment     (0),
  _use_run_info   (true),
  _reportTask     (new Task(TaskObject("controlRep"))),
  _tmo            (tmo)
{
  memset(_transition_env,0,TransitionId::NumberOf*sizeof(unsigned));
  memset(_transition_xtc,0,TransitionId::NumberOf*sizeof(Xtc*));

  pthread_mutex_init(&_target_mutex, NULL);
  pthread_cond_init (&_target_cond , NULL);
}

PartitionControl::~PartitionControl()
{
  _sequenceTask->destroy();
  _reportTask  ->destroy();
}

void PartitionControl::add_src_alias(const SrcAlias& alias)
{
// _src_aliases.insert(alias);
  _src_aliases.push_back(alias);
  _src_aliases.sort();
  _src_aliases.unique();
}

const char *PartitionControl::lookup_src_alias(const Src& src)
{
  std::list<SrcAlias>::iterator it;
  for (it = _src_aliases.begin(); it != _src_aliases.end(); it++) {
    if ((Src)*it == src) {
      return (it->aliasName());
    }
  }
  // no match found
  return ((char *)NULL);
}

void PartitionControl::platform_rollcall(PlatformCallback* cb)
{
  if ((_platform_cb = cb)) {
    // send Message::Alias before Message::Ping, so aliases are learned first
    Message alias(Message::Alias);
    mcast(alias);
    Message ping(Message::Ping);
    mcast(ping);
  }
}

bool PartitionControl::set_partition(const char* name,
                                     const char* dbpath,
                                     const Node* nodes,
                                     unsigned    nnodes,
                                     uint64_t    bldmask,
                                     uint64_t    bldmask_mon,
                                     unsigned    options)
{
  if (options&Allocation::ShortDisableTmo) {
    disable_tmo.tv_sec =0;
    disable_tmo.tv_nsec= 20000000;
  }
  else {
    disable_tmo.tv_sec =0;
    disable_tmo.tv_nsec=300000000;
  }

  _partition = Allocation(name,dbpath,partitionid(),bldmask,bldmask_mon,options);
  for(unsigned k=0; k<nnodes; k++)
    _partition.add(nodes[k]);
  return true;
}

bool PartitionControl::set_partition(const Allocation& alloc)
{
  _partition = alloc;
  return true;
}

const Allocation& PartitionControl::partition() const
{ return _partition; }

void PartitionControl::set_target_state(State state)
{
  if (state != _target_state || state != _target_state)
    printf("PartitionControl::set_target_state curr %s  prevtgt %s  tgt %s\n",
         name(_current_state), name(_target_state), name(state));

  State prev_target = _target_state;
  _target_state = state;
  if (prev_target == _current_state)
    _next();
}

void PartitionControl::wait_for_target()
{
  pthread_mutex_lock(&_target_mutex);
  while(_current_state != _target_state)
    pthread_cond_wait(&_target_cond, &_target_mutex);
}

void PartitionControl::release_target()
{
  pthread_mutex_unlock(&_target_mutex);
}

PartitionControl::State PartitionControl::target_state () const { return _target_state; }
PartitionControl::State PartitionControl::current_state() const { return _current_state; }

void PartitionControl::reconfigure(bool wait)
{
  printf("PartitionControl::reconfigure %c\n",wait?'t':'f');
  if (_target_state > Mapped) {
    _queued_target = _target_state;
    set_target_state(Mapped);
    if (wait) {
      pthread_mutex_lock(&_target_mutex);
      while(_current_state != _target_state)
        pthread_cond_wait(&_target_cond, &_target_mutex);
      pthread_mutex_unlock(&_target_mutex);
    }
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
  case Message::Alias:
    if (!_isallocated && _platform_cb) {
      _platform_cb->aliasCollect(hdr,static_cast<const AliasReply&>(msg));
    }
    break;
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
        //  reconfigure();
        break;
      case OccurrenceId::DataFileOpened: {
        const DataFileOpened& dfo = reinterpret_cast<const DataFileOpened&>(occ);
        printf("Received DataFileOpened occurrence from %x/%d [r%04d-s%02d-c%02d] host=%s path=%s\n",
               hdr.procInfo().ipAddr(),
               hdr.procInfo().processId(),
               dfo.run, dfo.stream, dfo.chunk, dfo.host, dfo.path);
        if (_runAllocator) {
          std::string hostname = dfo.host;
          std::string fname = dfo.path;
          _reportTask->call(new ReportJob(*_runAllocator, dfo.expt, dfo.run, dfo.stream, dfo.chunk, hostname, fname));
        }
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
#ifdef DBUG
  printf("PartitionControl::_next  current %s  target %s\n",
	 name(_current_state), name(_target_state));
#endif
  if      (_current_state==_target_state) {
    pthread_cond_signal(&_target_cond);
  }
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
#ifdef USE_L1A
      _queue(TransitionId::L1Accept);
#else
      _queue(TransitionId::Disable);
#endif
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

void PartitionControl::_eb_tmo_recovery()
{
  Transition* out = _eb.recover();
  if (out) {
    PartitionMember::message(header(),*out);
    delete out;
    _sem.give();
  }
  else {
    _tmo->routine();
  }
}

