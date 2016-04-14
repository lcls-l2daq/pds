#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <math.h>

#include "EvrSimManager.hh"
#include "EvrFifoServer.hh"
#include "EvrTimer.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/config/EvsConfigType.hh" // typedefs for the Evr config data types
#include "pds/config/CfgClientNfs.hh"

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/utility/Mtu.hh"
#include "pds/utility/ToNetEb.hh"
#include "pds/xtc/CDatagram.hh"

#include "pds/service/Client.hh"
#include "pds/service/Ins.hh"
#include "pds/collection/Route.hh"
#include "pds/utility/StreamPorts.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TimeStamp.hh"

static bool _randomize_nodes = false;

static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 80; 
static const int      giMaxCalibCycles  = 500;

namespace Pds {

  class SimEvrTimer;

  class Cancel : public Routine {
  public:
    Cancel(SimEvrTimer& timer) : _timer(timer), _sem(Semaphore::EMPTY) {}
  public:
    void routine();
    void wait   () { _sem.take(); }
  private:
    SimEvrTimer& _timer;
    Semaphore _sem;
  };

  class SimEvrTimer : public Routine {
  public:
    SimEvrTimer(Routine* cb) : 
      _task  (new Task(TaskObject("SimTimer"))), 
      _cb    (cb),
      _cancel(*this) 
    {}
    ~SimEvrTimer() { _task->destroy(); }
  public:
    void start( double sec ) 
    {
      _continue   = true;
      _dt  = sec;
      _adt = 0;
      _atv.tv_sec = 0;
      _task->call(this);
    }
    void cancel() { 
      _task->call(&_cancel); 
      _cancel.wait(); 
    }
    void routine()
    {
      struct timespec atv;
      clock_gettime(CLOCK_REALTIME,&atv);

      double delta = _dt;
      if (_atv.tv_sec) {
	double q = double(atv.tv_sec-_atv.tv_sec)+1.e-9*double(atv.tv_nsec-_atv.tv_nsec);
	_adt  += q-_dt;
	delta -= _adt;
      }
      _atv   = atv;

      if (delta>0) {
	struct timespec tv;
	tv.tv_sec  = time_t(delta);
	tv.tv_nsec = long  (1.e9*(delta-double(tv.tv_sec)));
	if (nanosleep(&tv,NULL)==0)
	  _cb->routine();
      }
      else
	_cb->routine();
      if (_continue)
	_task->call(this);
    }
  private:
    friend class Cancel;
    Task*    _task;
    Routine* _cb;
    double   _dt;
    double   _adt;
    struct timespec _atv;
    Cancel   _cancel;
    bool     _continue;
  };

  void Cancel::routine() { _timer._continue=false; _sem.give(); }

  class SimEvr : public Routine {
  public:
    SimEvr(EvrFifoServer& srv,
	   EvrTimer&      done) : _timer   (this),
				  _outlet  (0),
				  _srv     (srv),
				  _done    (done),
				  _count   (0),
				  _fiducial(0),
				  _swtrig_out(0) {}
    ~SimEvr() {}
  public:
    void     allocate (const Allocation& alloc) 
    {
      if (_outlet) delete _outlet;
      _outlet = 0;
      if (_swtrig_out) delete _swtrig_out;
      _swtrig_out = 0;
      _ldst.clear();

      //
      //  Test if we own the primary EVR for this partition
      //
      int pid = getpid();
      bool lmaster = true;
      unsigned nnodes = alloc.nnodes();
      for (unsigned n = 0; n < nnodes; n++) {
	const Node *node = alloc.node(n);
	if (node->level() == Level::Segment) {
	  if (node->pid() == pid) {
	    if (lmaster)
	      printf("Found master EVR\n");
	    else
	      printf("Found slave EVR\n");

	    unsigned partition = alloc.partitionid();
	    _outlet     = new Client(sizeof(EvrDatagram), 0, Ins(Route::interface()));
	    _swtrig_out = new ToNetEb(Route::interface(), Mtu::Size, 16);
	    _swtrig_dst = StreamPorts::event(partition,Level::Observer);
	    for(unsigned i=0; i<2; i++) {
	      _ldst.push_back(StreamPorts::event(partition,Level::Segment,i,
						 static_cast<const DetInfo&>(_srv.client()).devId()));
	      printf("SimEvr::allocate sending to %08x/%d\n",_ldst[i].address(),_ldst[i].portId());
	    }
	    break;
	  }
	  else
	    lmaster = false;
	}
      }
      _lmaster = lmaster;
    }
    void     reset  () { _count=0; }
    void     configure(unsigned prescale) { _duration=double(prescale)/119e6; }
    void     enable   (unsigned events) 
    {
      _evt_stop = _count + events;
      printf("SimEvr starting with duration %f sec\n",_duration);
      _timer.start(_duration); 
    }
    void     disable() 
    {
      printf("SimEvr stopping [%d]\n",_count);
      _timer.cancel(); 
      printf("SimEvr stopped\n");
    }
    void     routine() 
    {
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ClockTime ctime(ts.tv_sec, ts.tv_nsec);
      TimeStamp stamp(0,_fiducial, _count);

      Sequence seq(Sequence::Event, TransitionId::L1Accept, ctime, stamp);
      EvrDatagram datagram(seq, _count, 0);
      
      _srv.post(_count, _fiducial); 
      
      datagram.setL1AcceptEnv((1<<_ldst.size())-1);
      datagram.evr = _count;
      for(unsigned i=0; i<_ldst.size(); i++)
	_outlet->send((char *) &datagram, (char*)NULL, 0, _ldst[i]);
	
      _count++;
      _fiducial++;
      _fiducial %= TimeStamp::MaxFiducials;
      	
      if (_lmaster) {
	if (_count == _evt_stop) {
	  printf("Stopping at %u\n",_count);
	  _done.expired();
	}
      }
    }
    bool master() const { return _lmaster; }
    void forward(InDatagram* dg) 
    { 
      if (_lmaster)
	dg->send(*_swtrig_out,_swtrig_dst);
    }
  private:
    SimEvrTimer    _timer;
    bool           _lmaster;
    Client*        _outlet;
    std::vector<Ins> _ldst;
    EvrFifoServer& _srv;
    EvrTimer&      _done;
    unsigned       _count;
    unsigned       _fiducial;
    double         _duration;
    unsigned       _evt_stop;
    ToNetEb*       _swtrig_out;
    Ins            _swtrig_dst;
  };

  class EvrSimEnableAction:public Action
  {
  public:
    EvrSimEnableAction(SimEvr& er, EvrTimer& done, Appliance& app) : 
      _er(er), _done(done), _app(app), 
      _pool(sizeof(Occurrence),1), _events(0) {}

    Transition* fire(Transition* tr) { 
      const EnableEnv& env = static_cast<const EnableEnv&>(tr->env());
      if (env.timer()) {
	_done.set_duration_ms(env.duration());
	_done.start();
	_events = 0;
      }
      else
	_events = env.events();
      return tr; 
    }
    InDatagram* fire(InDatagram* tr) { 
      _er.enable(_events); 
      if (_er.master())
	_app.post(new (&_pool) Occurrence(OccurrenceId::EvrEnabled));
      return tr; 
    }
  private:
    SimEvr&     _er;
    EvrTimer&   _done;
    Appliance&  _app;
    GenericPool _pool;
    unsigned    _events;
  };

  class EvrSimDisableAction:public Action
  {
  public:
    EvrSimDisableAction(EvrTimer& done) : 
      _done(done) {}
    
    Transition* fire(Transition* tr) { 
      _done.cancel();
      return tr; 
    }
    InDatagram* fire(InDatagram* tr) { return tr; }
  private:
    EvrTimer& _done;
  };

  static unsigned int configSize(unsigned maxNumEventCodes, unsigned maxNumPulses, unsigned maxNumOutputMaps)
  {
    return (sizeof(EvsConfigType) + 
	    maxNumEventCodes * sizeof(EvsCodeType) +
	    maxNumPulses     * sizeof(PulseType) + 
	    maxNumOutputMaps * sizeof(OutputMapType));
  }

  class EvrSimL1Action : public Action {
  public:
    EvrSimL1Action(SimEvr& er, const Src& src) : 
      _er(er), 
      _tc(TypeId(TypeId::Id_EvrData,3),src)
    {
      EvrData::DataV3 d(1);
      _payload = new char[d._sizeof()];
      _tc.extent += d._sizeof();
    }
    ~EvrSimL1Action() { delete[] _payload; }
  public:
    Transition* fire(Transition* tr) { _er.disable(); return tr; }
    InDatagram* fire(InDatagram* dg) { 
      dg->datagram().xtc.extent = sizeof(Xtc); 
      if (_er.master()) {
	EvrData::FIFOEvent fe(dg->datagram().seq.stamp().fiducials(),0,39);
	new (_payload) EvrData::DataV3(1,&fe);
	dg->insert(_tc,_payload);
	_er.forward(dg);
      }
      return dg; 
    }
  private:
    SimEvr& _er;
    Xtc     _tc;
    char*   _payload;
  };

  class EvrSimConfigManager
  {
  public:
    EvrSimConfigManager(SimEvr& er, CfgClientNfs & cfg, Appliance& app):
      _er (er),
      _cfg(cfg),
      _cfgtc(_evsConfigType, cfg.src()),
      _configBuffer(
		    new char[ giMaxCalibCycles* configSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] ),
      _cur_config(0),
      _end_config(0),
      _occPool   (new GenericPool(sizeof(UserMessage),1)),
      _app       (app)
    {
    }

    ~EvrSimConfigManager()
    {
      delete   _occPool;
      delete[] _configBuffer;
    }

    void insert(InDatagram * dg)
    {
      dg->insert(_cfgtc, _cur_config);
    }

    void configure()
    {
      if (!_cur_config)
	return;

      unsigned nerror  = 0;
      UserMessage* msg = new(_occPool) UserMessage;
      msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
      msg->append(":");

      const EvsConfigType& cfg = *_cur_config;

      printf("Configuring evr : %d/%d/%d\n",
	     cfg.npulses(),cfg.neventcodes(),cfg.noutputs());

      /*
       * enable event codes, and setup each event code's pulse mapping
       */
      if (cfg.neventcodes() != 1) {
	nerror++;
	char buff[64];
	sprintf(buff,"EVR neventcodes(%d) != 1 for internal sequence setup.\n", cfg.neventcodes());
	msg->append(buff);
      }
      else {
	int uEventIndex=0;

	const EvsCodeType& eventCode = cfg.eventcodes()[uEventIndex];
              
	if (eventCode.period()) {
	  _er.configure( eventCode.period() );
	}

	printf("event %d : %d period %d %x/%x group %d\n",
	       uEventIndex, eventCode.code(), eventCode.period(),
	       eventCode.maskTriggerP(),
	       eventCode.maskTriggerR(),
	       eventCode.readoutGroup());       
      }

      if (nerror) {
	_app.post(msg);
	_cfgtc.damage.increase(Damage::UserDefined);
      }
      else
	delete msg;
    }

    //
    //  Fetch the explicit EVR configuration and the Sequencer configuration.
    //
    void fetch(Transition* tr)
    {
      UserMessage* msg = new(_occPool) UserMessage;
      msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
      msg->append(":");

      _cfgtc.damage = 0;
      _cfgtc.damage.increase(Damage::UserDefined);

      int len = _cfg.fetch(*tr, _evsConfigType, _configBuffer);
      if (len <= 0)
	{
	  _cur_config = 0;
	  _cfgtc.extent = sizeof(Xtc);
	  _cfgtc.damage.increase(Damage::UserDefined);
	  printf("Config::configure failed to retrieve Evr configuration\n");
	  msg->append("failed to read Evr configuration\n");
	  _app.post(msg);
	  return;
	}

      _cur_config = reinterpret_cast<const EvsConfigType*>(_configBuffer);
      _end_config = _configBuffer+len;
      _cfgtc.extent = sizeof(Xtc) + Pds::EvsConfig::size(*_cur_config);

      delete msg;
      _cfgtc.damage = 0;
    }

    void advance()
    {
      int len = _cur_config->_sizeof();
      const char* nxt_config = reinterpret_cast<const char*>(_cur_config)+len;
      if (nxt_config < _end_config)
	_cur_config = reinterpret_cast<const EvsConfigType*>(nxt_config);
      else
	_cur_config = reinterpret_cast<const EvsConfigType*>(_configBuffer);
      _cfgtc.extent = sizeof(Xtc) + _cur_config->_sizeof();
    }

  private:
    SimEvr&              _er;
    CfgClientNfs&        _cfg;
    Xtc                  _cfgtc;
    char *               _configBuffer;
    const EvsConfigType* _cur_config;
    const char*          _end_config;
    GenericPool*         _occPool;
    Appliance&           _app;
  };

  class EvrSimConfigAction: public Action {
  public:
    EvrSimConfigAction(SimEvr& er, EvrSimConfigManager& cfg) : _er(er), _cfg(cfg) {}
  public:
    Transition* fire(Transition* tr) { 
      _cfg.fetch(tr);
      _cfg.configure();
      _er .reset();
      return tr;
    }
    InDatagram* fire(InDatagram* dg) { 
      if (_er.master()) _cfg.insert(dg); 
      return dg; 
    }
  private:
    SimEvr& _er;
    EvrSimConfigManager& _cfg;
  };

  class EvrSimBeginCalibAction: public Action {
  public:
    EvrSimBeginCalibAction(EvrSimConfigManager& cfg) : _cfg(cfg) {}
  public:
    Transition* fire(Transition* tr) { _cfg.configure(); return tr; }
    InDatagram* fire(InDatagram* dg) { _cfg.insert(dg); return dg; }
  private:
    EvrSimConfigManager& _cfg;
  };

  class EvrSimEndCalibAction: public Action
  {
  public:
    EvrSimEndCalibAction(EvrSimConfigManager& cfg): _cfg(cfg) {}
  public:
    Transition *fire(Transition * tr) { return tr; }
    InDatagram *fire(InDatagram * dg) { _cfg.advance(); return dg; }
  private:
    EvrSimConfigManager& _cfg;
  };

  class EvrSimAllocAction:public Action
  {
  public:
    EvrSimAllocAction(CfgClientNfs& cfg, 
		      SimEvr& er) : _cfg(cfg), _er(er) {}
    Transition *fire(Transition * tr)
    {
      const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
      _cfg.initialize(alloc.allocation());
      _er .allocate(alloc.allocation());
      return tr;
    }
  private:
    CfgClientNfs&   _cfg;
    SimEvr&         _er;
  };
};

using namespace Pds;

Appliance & EvrSimManager::appliance()
{
  return _fsm;
}

Server& EvrSimManager::server()
{
  return *_server;
}

EvrSimManager::EvrSimManager(CfgClientNfs & cfg) :
  _fsm   (*new Fsm),
  _server(new EvrFifoServer(cfg.src())),
  _done  (new EvrTimer(_fsm)),
  _er    (*new SimEvr(*_server,*_done))
{
  EvrSimConfigManager* cmgr = new EvrSimConfigManager(_er, cfg, _fsm);

  _fsm.callback(TransitionId::Map            , new EvrSimAllocAction     (cfg,_er));
  _fsm.callback(TransitionId::Configure      , new EvrSimConfigAction    (_er,*cmgr));
  _fsm.callback(TransitionId::BeginCalibCycle, new EvrSimBeginCalibAction(*cmgr));
  _fsm.callback(TransitionId::EndCalibCycle  , new EvrSimEndCalibAction  (*cmgr));
  _fsm.callback(TransitionId::Enable         , new EvrSimEnableAction    (_er,*_done,_fsm));
  _fsm.callback(TransitionId::Disable        , new EvrSimDisableAction   (*_done));
  _fsm.callback(TransitionId::L1Accept       , new EvrSimL1Action        (_er,cfg.src()));
}

EvrSimManager::~EvrSimManager()
{
  delete _done;
}

void EvrSimManager::randomize_nodes(bool v) { _randomize_nodes=v; }

