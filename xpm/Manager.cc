#include "pds/xpm/Manager.hh"
#include "pds/xpm/Server.hh"
#include "pds/cphw/RingBuffer.hh"

#include "pds/config/TriggerConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescScalar.hh"
#include "pds/mon/MonEntryScalar.hh"

#include "pds/xpm/Module.hh"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace Pds {
  namespace Xpm {
    //  This should record the official time for each event
    //  and any global trigger information
    class L1Action : public Action {
    public:
      L1Action(Module& dev) : _dev(dev) {}
      InDatagram* fire(InDatagram* in) {
        return in;
      }
      Transition* fire(Transition* in) {
        _dev.setL0Enabled(false);
        printf("L0T off\n");
        return in;
      }
    private:
      Module& _dev;
    };
    class EnableAction : public Action {
    public:
      EnableAction(Module& dev,
                   L0Stats& s) : _dev(dev), _s(s) {}
      Transition* fire(Transition* in) { return in; }
      InDatagram* fire(InDatagram* in) {
        _s = _dev.l0Stats();
        _s.dump();
        _dev.setL0Enabled(true);
        printf("L0T on [%x]\n",unsigned(_dev._enabled));
        return in;
      }
    private:
      Module&  _dev;
      L0Stats& _s;
    };
    class DisableAction : public Action {
    public:
      DisableAction(Module& dev,
                    L0Stats& s) : _dev(dev), _s(s) {}
      Transition* fire(Transition* in) {
        L0Stats s = _dev.l0Stats();
        s.dump();
#define PDIFF(title,stat) //printf("%9.9s: %lu\n",#title,s.stat-_s.stat)
        PDIFF(Enabled  ,l0Enabled);
        PDIFF(Inhibited,l0Inhibited);
        PDIFF(L0,numl0);
        PDIFF(L0Inh,numl0Inh);
        PDIFF(L0Acc,numl0Acc);
        PDIFF(rxErrs,rx0Errs);
#undef PDIFF
        return in;
      }
    private:
      Module&  _dev;
      L0Stats& _s;
    };

    class ConfigAction : public Action {
      enum {MaxConfigSize=0x1000};
    public:
      ConfigAction(Xpm::Module&      dev,
                   const Src&        src,
                   Manager&          mgr,
                   CfgClientNfs&     cfg,
                   const Allocation& alloc) :
        _dev    (dev),
        _config (new char[MaxConfigSize]),
//         _cfgtc  (_generic1DConfigType,src),
        _mgr    (mgr),
        _cfg    (cfg),
        _alloc  (alloc),
        _occPool(sizeof(UserMessage),1) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) 
      {
//         dg->insert(_cfgtc, &_config);
        if (_nerror) {
          printf("*** Found %d configuration errors\n",_nerror);
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) 
      {
        _nerror = 0;

        int len     = _cfg.fetch( *tr,
                                  _trgConfigType,
                                  _config,
                                  MaxConfigSize );
        
        if (len) {
          TriggerConfigType& c = *reinterpret_cast<TriggerConfigType*>(_config);    
          _dev.setL0Select_FixedRate(c.l0Select()[0].fixedRate()); 
        }
          
        for(unsigned i=0; i<_alloc.nnodes(); i++) {
          unsigned paddr = _alloc.node(i)->paddr();
          if (paddr&0xffff0000)
            continue;
          _dev.rxLinkReset(paddr&0xf);
        }
        usleep(1000);

        _dev.init();
        printf("rx/tx Status: %08x/%08x\n", 
               _dev.rxLinkStat(), _dev.txLinkStat());

        _dev.resetL0(true);
        bool lenable=true;

        for(unsigned i=0; i<_alloc.nnodes(); i++) {
          unsigned paddr = _alloc.node(i)->paddr();
          if (paddr&0xffff0000)
            continue;
          _dev.linkEnable(paddr&0xf,lenable);
        }
        
        printf("Configuration Done\n");
        
        _nerror = 0;  // override
        
        if (_nerror) {
          
          UserMessage* msg = new (&_occPool) UserMessage;
          msg->append("Xpm: failed to apply configuration.\n");
          _mgr.appliance().post(msg);
          
        }
        return tr;
      }
    private:
      Xpm::Module&        _dev;
      char*               _config;
//       Xtc                 _cfgtc;
      Manager&            _mgr;
      CfgClientNfs&       _cfg;
      const Allocation&   _alloc;
      GenericPool         _occPool;
      unsigned            _nerror;
    };
    class StatsTimer : public Timer {
    public:
      StatsTimer(Module& dev);
      ~StatsTimer() { _task->destroy(); }
    public:
      void partition(unsigned p) { _partition=p; }
      void expired();
      Task* task() { return _task; }
      unsigned duration  () const { return 1000; }
      unsigned repetitive() const { return 1; }
    private:
      Module&         _dev;
      Task*           _task;
      MonEntryScalar* _stats;
      unsigned        _partition;
      L0Stats         _s;
    };
    class MapAction : public Action {
    public:
      MapAction(Allocation& alloc, 
                CfgClientNfs& cfg,
                StatsTimer& t) : _alloc(alloc), _cfg(cfg), _t(t) {}
      ~MapAction() {}
    public:
      InDatagram* fire(InDatagram* dg) { return dg; }
      Transition* fire(Transition* tr) {
        const Allocation& alloc = static_cast<Allocate*>(tr)->allocation();
        _cfg.initialize(alloc);
        _t.partition(alloc.partitionid());
        _alloc = alloc;
        return tr;
      }
    private:
      Allocation& _alloc;
      CfgClientNfs& _cfg;
      StatsTimer& _t;
    };
    class BeginRun : public Action {
    public:
      BeginRun(StatsTimer& t) : _t(t) {}
      ~BeginRun() {}
    public:
      InDatagram* fire(InDatagram* dg) { return dg; }
      Transition* fire(Transition* tr) {
        _t.start();
        return tr;
      }
    private:
      StatsTimer& _t;
    };
    class EndRun : public Action {
    public:
      EndRun(StatsTimer& t) : _t(t) {}
      ~EndRun() {}
    public:
      InDatagram* fire(InDatagram* dg) { return dg; }
      Transition* fire(Transition* tr) {
        _t.cancel();
        return tr;
      }
    private:
      StatsTimer& _t;
    };
  }
}

using namespace Pds::Xpm;

StatsTimer::StatsTimer(Module& dev) :
  _dev      (dev),
  _task     (new Task(TaskObject("PtnS")))
{
  MonGroup* group = new MonGroup("Xpm");
  VmonServerManager::instance()->cds().add(group);

  { std::vector<std::string> names;
    names.push_back("Enabled");   _s.l0Enabled=0;
    names.push_back("Inhibited"); _s.l0Inhibited=0;
    names.push_back("L0T");       _s.numl0=0;
    names.push_back("L0TInh");    _s.numl0Inh=0;
    names.push_back("L0TAcc");    _s.numl0Acc=0;
    names.push_back("RxErrs");    _s.rx0Errs=0;
    MonDescScalar desc("L0Stats",names);
    _stats = new MonEntryScalar(desc);
    group->add(_stats); }
}

void StatsTimer::expired()
{
  timespec t; clock_gettime(CLOCK_REALTIME,&t);
  L0Stats s = _dev.l0Stats();
#define INCSTAT(v,i) _stats->addvalue(s.v-_s.v, i)
  INCSTAT(l0Enabled  ,0);
  INCSTAT(l0Inhibited,1);
  INCSTAT(numl0      ,2);
  INCSTAT(numl0Inh   ,3);
  INCSTAT(numl0Acc   ,4);
  INCSTAT(rx0Errs    ,5);
#undef INCSTAT
  _stats->time(ClockTime(t));
  //  s.dump();
  _s=s;
}


Manager::Manager(Module& dev, Server& server, CfgClientNfs& cfg) : _fsm(*new Fsm())
{
  L0Stats* s = new L0Stats;

  _stats = new StatsTimer(dev);
  _alloc = new Allocation;
  _fsm.callback(TransitionId::Map      ,new MapAction    (*_alloc, cfg, *_stats));
  _fsm.callback(TransitionId::Configure,new ConfigAction (dev, server.client(), *this, cfg, *_alloc));
  _fsm.callback(TransitionId::BeginRun ,new BeginRun     (*_stats));
  _fsm.callback(TransitionId::Enable   ,new EnableAction (dev,*s));
  _fsm.callback(TransitionId::Disable  ,new DisableAction(dev,*s));
  _fsm.callback(TransitionId::L1Accept ,new L1Action     (dev));
  _fsm.callback(TransitionId::EndRun   ,new EndRun       (*_stats));
}

Manager::~Manager() 
{
  delete _stats;
  delete _alloc;
}

Pds::Appliance& Manager::appliance() {return _fsm;}

