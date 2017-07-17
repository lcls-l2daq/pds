#include "pds/xpm/Manager.hh"
#include "pds/xpm/Server.hh"

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

#include "pds/xpm/Module.hh"
#include "pds/tag/Server.hh"

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
  namespace Xpm {
    class TagServer : public Pds::Tag::ServerImpl {
    public:
      TagServer(Module& dev) : _dev(dev)
      {
        _dev._analysisRst = 0xffff;
        _dev._analysisRst = 0;
        _anar[0] = _anar[1] = 0;
        _anac[0] = _anac[1] = -1;
      }
    public:
      void insert(Pds::Tag::Key key, unsigned buffer) {
        unsigned v = key.mask() | key.tag(buffer);
#ifdef DBUG
        unsigned g = key.member();
        if (buffer != ((_anac[g]+1)&0xf))
            printf("Xpm::TagServer inserting %x  anac %x  anar %x\n",
                   buffer, _anac[g], _anar[g]);
        _anac[g] = buffer;
        _anar[g]++;
#endif
        _dev._analysisTag = v;
      }
    private:
      Module& _dev;
      unsigned _anar[2];
      unsigned _anac[2];
    };

    class PVStats {
    public:
      PVStats() : _pv(0) {}
      ~PVStats() {}
    public:
      void allocate(const Allocation& alloc) {
        if (ca_current_context() == NULL) {
          printf("Initializing context\n");
          SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
                   "Calling ca_context_create" );
        }

        for(unsigned i=0; i<_pv.size(); i++)
          delete _pv[i];
        _pv.resize(0);

        std::ostringstream o;
        o << "DAQ:" << alloc.partition() << ":";
        std::string pvbase = o.str();
        _pv.push_back( new PVWriter((pvbase+"L0INPRATE").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"L0ACCRATE").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"L1RATE").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"NUML0INP" ).c_str()) );
        _pv.push_back( new PVWriter((pvbase+"NUML0ACC" ).c_str()) );
        _pv.push_back( new PVWriter((pvbase+"NUML1" ).c_str()) );
        _pv.push_back( new PVWriter((pvbase+"DEADFRAC").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"DEADTIME").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"DEADFLNK").c_str(),8) );

        _pv.push_back( new PVWriter((pvbase+"RXCLKCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"TXCLKCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"RXRSTCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"CRCERRCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"RXDECERRCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"RXDSPERRCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"BYPASSRESETCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"BYPASSDONECOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"RXLINKUP").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"FIDCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"SOFCOUNT").c_str()) );
        _pv.push_back( new PVWriter((pvbase+"EOFCOUNT").c_str()) );
        printf("PVs allocated\n");
      }
      void begin(const L0Stats& s)
      {
        _begin=s;
        printf("Begin NumL0 %lu  Acc %lu\n",
               s.numl0, s.numl0Acc);
      }
    public:
#define PVPUT(i,v) { *reinterpret_cast<double*>(_pv[i]->data()) = double(v); _pv[i]->put(); }
      void update(const L0Stats& ns, const L0Stats& os, double dt)
      { unsigned numl0    = ns.numl0-os.numl0;
        PVPUT(0,double(numl0)/dt);
        unsigned numl0Acc = ns.numl0Acc-os.numl0Acc;
        PVPUT(1,double(numl0Acc)/dt);
        PVPUT(3,ns.numl0   -_begin.numl0);
        PVPUT(4,ns.numl0Acc-_begin.numl0Acc);
        PVPUT(6,numl0    ?double(ns.numl0Inh-os.numl0Inh)/double(numl0):0);
        unsigned l0Enabled= ns.l0Enabled-os.l0Enabled;
        PVPUT(7,l0Enabled?double(ns.l0Inhibited-os.l0Inhibited)/double(l0Enabled):0);
        for(unsigned i=0; i<8; i++) {
          reinterpret_cast<double*>(_pv[8]->data())[i] = double(ns.linkInh[i]-os.linkInh[i])/double(l0Enabled);
        }
        _pv[8]->put();
        ca_flush_io(); }
      void update(const CoreCounts& nc, const CoreCounts& oc, double dt)
      {
        PVPUT( 9, double(nc.rxClkCount       - oc.rxClkCount      ) / dt);
        PVPUT(10, double(nc.txClkCount       - oc.txClkCount      ) / dt);
        PVPUT(11, double(nc.rxRstCount       - oc.rxRstCount      ) / dt);
        PVPUT(12, double(nc.crcErrCount      - oc.crcErrCount     ) / dt);
        PVPUT(13, double(nc.rxDecErrCount    - oc.rxDecErrCount   ) / dt);
        PVPUT(14, double(nc.rxDspErrCount    - oc.rxDspErrCount   ) / dt);
        PVPUT(15, double(nc.bypassResetCount - oc.bypassResetCount) / dt);
        PVPUT(16, double(nc.bypassDoneCount  - oc.bypassDoneCount ) / dt);
        PVPUT(17, double(nc.rxLinkUp                              )     );
        PVPUT(18, double(nc.fidCount         - oc.fidCount        ) / dt);
        PVPUT(19, double(nc.sofCount         - oc.sofCount        ) / dt);
        PVPUT(20, double(nc.eofCount         - oc.eofCount        ) / dt);
        //ca_flush_io();  // Maybe let the L0Stats update do the flush...
      }
    private:
      std::vector<PVWriter*> _pv;
      L0Stats _begin;
    };

    class PvAllocate : public Routine {
    public:
      PvAllocate(PVStats& pv,
                 const Allocation& alloc,
                 const L0Stats& s) :
        _pv(pv), _alloc(alloc), _s(s) {}
    public:
      void routine() {
        _pv.allocate(_alloc);
        //        _pv.update(_s,_s);
        delete this;
      }
    private:
      PVStats&   _pv;
      Allocation _alloc;
      L0Stats    _s;
    };

    class StatsTimer : public Timer {
    public:
      StatsTimer(Core& core, Module& dev);
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
      Core&           _core;
      Module&         _dev;
      Task*           _task;
      MonEntryScalar* _stats;
      CoreCounts      _c;
      L0Stats         _s;
      timespec        _t;
      PVStats         _pv;
    };

    class XpmAppliance : public Appliance {
      enum { MaxConfigSize=0x100000 };
    public:
      XpmAppliance(Module&       dev,
                   Server&       srv,
                   CfgClientNfs& cfg) :
        _core   (Core::get()),
        _dev    (dev),
        _cfg    (cfg),
        _config (new char[MaxConfigSize]),
//         _cfgtc  (_generic1DConfigType,src),
        _occPool(sizeof(UserMessage),1),
        _timer  (_core, dev) {}
    public:
      Transition* transitions(Transition* tr) {
        switch(tr->id()) {
        case TransitionId::L1Accept:
          _dev.setL0Enabled(false);
          break;
        case TransitionId::Disable:
          { CoreCounts c = _core.counts();
            c.dump();
            L0Stats s = _dev.l0Stats();
            s.dump();
#define PDIFF(title,stat) //printf("%9.9s: %lu\n",#title,s.stat-_enableStats.stat)
            PDIFF(Enabled  ,l0Enabled);
            PDIFF(Inhibited,l0Inhibited);
            PDIFF(L0,numl0);
            PDIFF(L0Inh,numl0Inh);
            PDIFF(L0Acc,numl0Acc);
            PDIFF(rxErrs,rx0Errs);
#undef PDIFF
          }
          break;
        case TransitionId::BeginRun:
          _timer.start();
          break;
        case TransitionId::EndRun:
          _timer.cancel();
          printf("analysisTag writes %x  reads %x\n",
                 unsigned(_dev._analysisTagWr),
                 unsigned(_dev._analysisTagRd));
          break;
        case TransitionId::Configure:
          { _nerror = 0;

            int len     = _cfg.fetch( *tr,
                                      _trgConfigType,
                                      _config,
                                      MaxConfigSize );

            if (len) {
              const TriggerConfigType& c = *reinterpret_cast<const TriggerConfigType*>(_config);
              const L0SelectType& l0t = c.l0Select()[0];
              switch(l0t.rateSelect()) {
              case L0SelectType::_FixedRate:
                _dev.setL0Select_FixedRate(l0t.fixedRate());
                break;
              case L0SelectType::_PowerSyncRate:
                _dev.setL0Select_ACRate   (l0t.powerSyncRate(),
                                           l0t.powerSyncMask());
                break;
              case L0SelectType::_ControlSequence:
                _dev.setL0Select_Sequence (l0t.controlSeqNum(),
                                           l0t.controlSeqBit());
                break;
              case L0SelectType::_EventCode:  // Firmware doesn't support this yet
                //             _dev.setL0Select_EventCode(l0t.eventCode());
                //            break;
              default:
                break;
              }
            }

            _alloc.dump();

            _dev.clearLinks();
            /*
            for(unsigned i=0; i<_alloc.nnodes(); i++) {
              unsigned paddr = _alloc.node(i)->paddr();
              if (paddr&0xffff0000)
                continue;
              _dev.txLinkReset(paddr&0xf);
              _dev.rxLinkReset(paddr&0xf);
            }
            usleep(10000);
            */
            _dev.resetL0(true);
            bool lenable=true;

            for(unsigned i=0; i<_alloc.nnodes(); i++) {
              unsigned paddr = _alloc.node(i)->paddr();
              if (paddr&0xffff0000)
                continue;
              _dev.linkEnable(paddr&0xf,lenable);
            }

            _dev.init();
            for(unsigned i=0; i<7; i++) { // Revisit: How many links?
                printf("rx/tx %d Status: %08x/%08x\n",
                       i, _dev.rxLinkStat(i), _dev.txLinkStat(i));
              }

            printf("Configuration Done\n");

            _nerror = 0;  // override

            if (_nerror) {
              UserMessage* msg = new (&_occPool) UserMessage;
              msg->append("Xpm: failed to apply configuration.\n");
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
          _core.counts().dump();
          (_enableStats=_dev.l0Stats()).dump();
          _dev.setL0Enabled(true);
          break;
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
      Core&         _core;
      Module&       _dev;
      CfgClientNfs& _cfg;
      char*         _config;
      Allocation    _alloc;
      GenericPool   _occPool;
      StatsTimer    _timer;
      L0Stats       _enableStats;
      unsigned      _nerror;
    };
  }
}

using namespace Pds::Xpm;

StatsTimer::StatsTimer(Core& core, Module& dev) :
  _core     (core),
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

void StatsTimer::allocate(const Allocation& alloc)
{
  clock_gettime(CLOCK_REALTIME,&_t);
  _s.l0Enabled=0;
  _s.l0Inhibited=0;
  _s.numl0=0;
  _s.numl0Inh=0;
  _s.numl0Acc=0;
  _s.rx0Errs=0;
  _task->call(new PvAllocate(_pv,alloc,_s));
}

void StatsTimer::start()
{
  _core.resetStats();

  _pv.begin(_dev.l0Stats());
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
  CoreCounts c = _core.counts();
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
  //c.dump();
  //s.dump();
  double dt = double(t.tv_sec-_t.tv_sec)+1.e-9*(double(t.tv_nsec)-double(_t.tv_nsec));
  _pv.update(c,_c,dt);
  _pv.update(s,_s,dt);
  _c=c;
  _s=s;
  _t=t;
}


Manager::Manager(Module& dev, Server& server, CfgClientNfs& cfg) : _app(new Xpm::XpmAppliance(dev,server,cfg))
{
  (new Pds::Tag::Server(Pds::CollectionPorts::tagserver(0).portId(),
                        *new TagServer(dev)))->start();
}

Manager::~Manager()
{
  delete _app;
}

Pds::Appliance& Manager::appliance() {return *_app;}

