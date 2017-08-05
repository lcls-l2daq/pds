#include "pds/dti/Manager.hh"
#include "pds/dti/Server.hh"
#include "pds/dti/Module.hh"
#include "pds/dti/PVStats.hh"

#include "pds/config/TriggerConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescScalar.hh"
#include "pds/mon/MonEntryScalar.hh"

#include "pds/collection/CollectionPorts.hh"

#include "pds/epicstools/PVWriter.hh"
using Pds_Epics::PVWriter;

#include <string>
#include <sstream>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DBUG

namespace Pds {
  namespace Dti {

    class PvAllocate : public Routine {
    public:
      PvAllocate(PVStats& pv,
                 const Allocation& alloc,
                 const Stats& stats) :
        _pv(pv), _alloc(alloc), _stats(stats) {}
    public:
      void routine() {
        std::ostringstream o;
        o << "DAQ:" << _alloc.partition();
        std::string pvbase = o.str();
        _pv.allocate(pvbase);
        //        _pv.update(_stats,_stats);
        delete this;
      }
    private:
      PVStats&   _pv;
      Allocation _alloc;
      Stats      _stats;
    };

    class StatsTimer : public Timer {
    public:
      StatsTimer(Module& dev);
      ~StatsTimer() { _task->destroy(); }
    public:
      void allocate(const Allocation&);
      void start   ();
      void cancel  ();
      void expired ();
      Task* task() { return _task; }
      //      unsigned duration  () const { return 1000; }
      unsigned duration  () const { return 1010; }  // 1% error on timer
      unsigned repetitive() const { return 1; }
    private:
      Module&         _dev;
      Task*           _task;
      MonEntryScalar* _stats;
      Stats           _s;
      timespec        _t;
      PVStats         _pv;
    };

    class DtiAppliance : public Appliance {
      enum { MaxConfigSize=0x100000 };
    public:
      DtiAppliance(Module&       dev,
                   Server&       srv,
                   CfgClientNfs& cfg) :
        _dev    (dev),
        _cfg    (cfg),
        _config (new char[MaxConfigSize]),
//         _cfgtc  (_generic1DConfigType,src),
        _usLinks(0),
        _occPool(sizeof(UserMessage),1),
        _timer  (dev) {}
    public:
      Transition* transitions(Transition* tr) {
        switch(tr->id()) {
        case TransitionId::L1Accept:
          {
            unsigned links = _usLinks;
            for (unsigned i = 0; links; ++i) {
              if (links & (1 << i)) {
                _dev._usLinkConfig[i].l1Enable(false);
                links &= ~(1 << i);
              }
            }
            break;
          }
        case TransitionId::Disable:
          {
            Stats stats = _dev.stats();
            stats.dump();
#define PDIFF(title,i,stat) //printf("%9.9s%d: %lu\n",title,i,stats.stat-_enableStats.stat)
            for (unsigned i = 0; i < Module::NUsLinks; ++i)
            {
              PDIFF("usRxErrs", i, us[i].rxErrs);
              PDIFF("usRxFull", i, us[i].rxFull);
              PDIFF("usIbRecv", i, us[i].ibRecv);
              PDIFF("usIbEvt",  i, us[i].ibEvt);
              PDIFF("usObRecv", i, us[i].obRecv);
              PDIFF("usObSent", i, us[i].obSent);
            }
            PDIFF("bpObSent", 16, bpObSent); // Revisit: bogus index
            for (unsigned i = 0; i < Module::NDsLinks; ++i)
            {
              PDIFF("dsRxErrs", i, ds[i].rxErrs);
              PDIFF("dsRxFull", i, ds[i].rxFull);
              PDIFF("dsObSent", i, ds[i].obSent);
            }
#undef PDIFF
          }
          break;
        case TransitionId::BeginRun:
          _timer.start();
          break;
        case TransitionId::EndRun:
          _timer.cancel();
          break;
        case TransitionId::Configure:
          {
            _nerror = 0;

            // Revisit: Not sure what to do here:
            //int len = _cfg.fetch( *tr,
            //                      _trgConfigType,
            //                      _config,
            //                      MaxConfigSize );
            //
            //if (len) {
            //  const TriggerConfigType& c = *reinterpret_cast<const TriggerConfigType*>(_config);
            //  const L0SelectType& l0t = c.l0Select()[0];
            //  switch(l0t.rateSelect()) {
            //  case L0SelectType::_FixedRate:
            //    _dev.setL0Select_FixedRate(l0t.fixedRate());
            //    break;
            //  case L0SelectType::_PowerSyncRate:
            //    _dev.setL0Select_ACRate   (l0t.powerSyncRate(),
            //                               l0t.powerSyncMask());
            //    break;
            //  case L0SelectType::_ControlSequence:
            //    _dev.setL0Select_Sequence (l0t.controlSeqNum(),
            //                               l0t.controlSeqBit());
            //    break;
            //  case L0SelectType::_EventCode:  // Firmware doesn't support this yet
            //    //             _dev.setL0Select_EventCode(l0t.eventCode());
            //    //            break;
            //  default:
            //    break;
            //  }
            //}

            _usLinks = 0x1;             // Revisit: Where does this mask come from?

            _alloc.dump();

            unsigned links = _usLinks;
            for (unsigned i = 0; links; ++i) {
              if (links & (1 << i)) {
                _dev._usLinkConfig[i].enable(true);
                _dev._usLinkConfig[i].tagEnable(true);
                links &= ~(1 << i);
              }
            }
            // Revisit: _dev.resetL0(true);
            // bool lenable=true;

            // Revisit:
            //for(unsigned i=0; i<_alloc.nnodes(); i++) {
            //  unsigned paddr = _alloc.node(i)->paddr();
            //  if (paddr&0xffff0000)
            //    continue;
            //  _dev.linkEnable(paddr&0xf,lenable);
            //}

            _dev.init();

            printf("Dti: Configuration Done\n");

            _nerror = 0;  // override

            if (_nerror) {
              UserMessage* msg = new (&_occPool) UserMessage;
              msg->append("Dti: failed to apply configuration.\n");
              post(msg);
            }
          }
          break;
        case TransitionId::Map:
          { const Allocation& alloc = static_cast<Allocate*>(tr)->allocation();
            _cfg.initialize(alloc);
            _timer.allocate(alloc);
            _alloc = alloc; }
          break;
        default:
          break;
        }
        return tr;
      }
    public:
      InDatagram* events     (InDatagram* dg) {
        switch(dg->datagram().seq.service()) {
        case TransitionId::L1Accept:
          break;
        case TransitionId::Enable:
          {
            (_enableStats=_dev.stats()).dump();
            unsigned links = _usLinks;
            for (unsigned i = 0; links; ++i) {
              if (links & (1 << i)) {
                _dev._usLinkConfig[i].l1Enable(true);
                links &= ~(1 << i);
              }
            }
            break;
          }
        case TransitionId::Configure:
//         dg->insert(_cfgtc, &_config);
          if (_nerror) {
            printf("*** Found %d configuration errors\n",_nerror);
            dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
          }
        default:
          break;
        }
        return dg;
      }
    private:
      Module&       _dev;
      CfgClientNfs& _cfg;
      char*         _config;
      unsigned      _usLinks;
      Allocation    _alloc;
      GenericPool   _occPool;
      StatsTimer    _timer;
      Stats         _enableStats;
      unsigned      _nerror;
    };
  }
}

using namespace Pds::Dti;

StatsTimer::StatsTimer(Module& dev) :
  _dev      (dev),
  _task     (new Task(TaskObject("PtnS")))
{
  MonGroup* group = new MonGroup("Dti");
  VmonServerManager::instance()->cds().add(group);

  {
    std::vector<std::string> names;

    for (unsigned i = 0; i < Module::NUsLinks; ++i)
    {
      std::ostringstream str;
      str << i;
      std::string idx = str.str();
      names.push_back("UsRxErrs"+idx);   _s.us[i].rxErrs=0;
      names.push_back("UsRxFull"+idx);   _s.us[i].rxFull=0;
      names.push_back("UsIbRecv"+idx);   _s.us[i].ibRecv=0;
      names.push_back("UsIbEvt"+idx);    _s.us[i].ibEvt=0;
      names.push_back("UsObRecv"+idx);   _s.us[i].obRecv=0;
      names.push_back("UsObSent"+idx);   _s.us[i].obSent=0;
    }
    names.push_back("BpObSent");         _s.bpObSent=0;
    for (unsigned i = 0; i < Module::NDsLinks; ++i)
    {
      std::ostringstream str;
      str << i;
      std::string idx = str.str();
      names.push_back("DsRxErrs"+idx);   _s.ds[i].rxErrs=0;
      names.push_back("DsRxFull"+idx);   _s.ds[i].rxFull=0;
      names.push_back("DsObSent"+idx);   _s.ds[i].obSent=0;
    }
    MonDescScalar desc("Stats",names);
    _stats = new MonEntryScalar(desc);
    group->add(_stats);
  }
}

void StatsTimer::allocate(const Allocation& alloc)
{
  clock_gettime(CLOCK_REALTIME,&_t);
  for (unsigned i = 0; i < Module::NUsLinks; ++i)
  {
    _s.us[i].rxErrs=0;
    _s.us[i].rxFull=0;
    _s.us[i].ibRecv=0;
    _s.us[i].ibEvt=0;
    _s.us[i].obRecv=0;
    _s.us[i].obSent=0;
  }
  _s.bpObSent = 0;
  for (unsigned i = 0; i < Module::NDsLinks; ++i)
  {
    _s.ds[i].rxErrs=0;
    _s.ds[i].rxFull=0;
    _s.ds[i].obSent=0;
  }
  _task->call(new PvAllocate(_pv,alloc,_s));
}

void StatsTimer::start()
{
  Timer::start();
}

void StatsTimer::cancel()
{
  Timer::cancel();
  expired();
}

//
//  Update VMON plots and EPICS PVs
//
void StatsTimer::expired()
{
  timespec t; clock_gettime(CLOCK_REALTIME,&t);
  Stats    s = _dev.stats();
  unsigned m = 0;
#define INCSTAT(v,i) _stats->addvalue(s.v-_s.v, i)
  for (unsigned i = 0; i < Module::NUsLinks; ++i)
  {
    INCSTAT(us[i].rxErrs, m++);
    INCSTAT(us[i].rxFull, m++);
    INCSTAT(us[i].ibRecv, m++);
    INCSTAT(us[i].ibEvt,  m++);
    INCSTAT(us[i].obRecv, m++);
    INCSTAT(us[i].obSent, m++);
  }
  INCSTAT(bpObSent, m++);
  for (unsigned i = 0; i < Module::NDsLinks; ++i)
  {
    INCSTAT(ds[i].rxErrs, m++);
    INCSTAT(ds[i].rxFull, m++);
    INCSTAT(ds[i].obSent, m++);
  }
#undef INCSTAT
  _stats->time(ClockTime(t));
  //s.dump();
  double dt = double(t.tv_sec-_t.tv_sec)+1.e-9*(double(t.tv_nsec)-double(_t.tv_nsec));
  _pv.update(s,_s,dt);
  _s=s;
  _t=t;
}


Manager::Manager(Module& dev, Server& server, CfgClientNfs& cfg) :
  _app(new Dti::DtiAppliance(dev, server, cfg))
{
}

Manager::~Manager()
{
  delete _app;
}

Pds::Appliance& Manager::appliance() {return *_app;}
