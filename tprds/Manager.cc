#include "pds/tprds/Manager.hh"
#include "pds/tprds/Server.hh"

#include "pds/config/Generic1DConfigType.hh"
#include "pds/config/TprDSConfigType.hh"
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

#include "pds/tag/Client.hh"
#include "pds/tag/Key.hh"
#include "pds/collection/CollectionPorts.hh"

#include "pds/tprds/Module.hh"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>

#define DBUG

static unsigned nsamples  [] = { 20 };
static unsigned sampletype[] = { Generic1DConfigType::UINT32 };
static int      offset    [] = { 0 };
static double   period    [] = { 8.e-9 };
static unsigned nPrint = 0;

namespace Pds {
  namespace TprDS {
    class StatsTimer : public Timer {
    public:
      StatsTimer(TprReg& dev,
                 MonGroup& group);
      ~StatsTimer() { _task->destroy(); }
    public:
      void expired();
      Task* task() { return _task; }
      //      unsigned duration  () const { return 1000; }
      unsigned duration  () const { return 1010; }
      unsigned repetitive() const { return 1; }
    private:
      TprReg&         _dev;
      Task*           _task;
      MonEntryScalar* _stats;
      MonEntryScalar* _rxlink;
      MonEntryScalar* _dma;
      class MyStats {
      public:
        unsigned evtCount;
        unsigned pauseCount;
        unsigned overflowCount;
        unsigned frameCount;
        unsigned rxRstDone;
        unsigned rxDecErrs;
        unsigned rxDspErrs;
      } _s;
    };

    class AnaTag {
    public:
      AnaTag(TprDS::TprReg& dev) : _dev(dev), _client(0), _anaw(0), _anar(-1), _anac(-1) {}
    public:
      void allocate(const Allocation& alloc) 
      {
        _client = new Tag::Client( Ins(alloc.master()->ip(), 
                                       CollectionPorts::tagserver(0).portId()),
                                   0, 1 );
        for(unsigned i=0; i<128; i++)
          _client->request(_anaw++&0xf);
      }
      void deallocate() { delete _client; }
      void analyze(const Datagram& dg, unsigned& tags, unsigned& errs) 
      {
        tags=errs=0;
        int index = _client->key().buffer(dg.env.value());
        if (index >= 0) {
          tags=1;
          if ((unsigned)index != ((_anac+1)&0xf)) {
#ifdef DBUG
            printf("AnaTag::analyze env %08x  indx %x  anac %x  anar %x  anaw %x\n", 
                   dg.env.value(), index, _anac, _anar, _anaw);
            printf(" -- rst %x  dsp %x  dec %x  crc %x\n",
                   unsigned(_dev.tpr.RxRstDone),
                   unsigned(_dev.tpr.RxDecErrs),
                   unsigned(_dev.tpr.RxDspErrs),
                   unsigned(_dev.tpr.CRCerrors));
#endif
            errs=1;
          }
          _anac = index;
          _anar++;
          
          _client->request(_anaw++&0xf);
        }
      }
      void dump() const { 
        printf("  anac %x  anar %x  anaw %x\n",
               _anac, _anar, _anaw);
      }
    private:
      TprDS::TprReg& _dev;
      Tag::Client* _client;
      unsigned _anaw;
      unsigned _anar;
      unsigned _anac;
    };

    class L1Action : public Action {
    public:
      L1Action(MonGroup& group,
               AnaTag&   tag) : _tag(tag) { 
        { std::vector<std::string> names;
          names.push_back("Events");
          names.push_back("AnaTags");
          MonDescScalar desc("L1Stats",names);
          _stats = new MonEntryScalar(desc);
          group.add(_stats); }
        { std::vector<std::string> names;
          names.push_back("TagMisses");
          names.push_back("RepeatPIDs");
          names.push_back("Corrupt");
          names.push_back("AnaErrs");
          MonDescScalar desc("L1Errors",names);
          _errors = new MonEntryScalar(desc);
          group.add(_errors); }
        reset(); 
      }
      void reset() { _ntag=-1; _pid=0; }
      InDatagram* fire(InDatagram* in) {
#if 0
        return 0;
#endif
        if (in->datagram().xtc.damage.value() ||
            in->datagram().xtc.extent != 2*sizeof(Xtc)+(nsamples[0]+1)*4) {
          _errors->addvalue(1,2); // corruptFrames++;
          if (nPrint) {
            nPrint--;
            printf("corrupt: dmg %x  extent %x\n",
                   in->datagram().xtc.damage.value(),
                   in->datagram().xtc.extent);
            const unsigned* p = reinterpret_cast<const unsigned*>(&in->datagram());
            const unsigned* end = reinterpret_cast<const unsigned*>(in->datagram().xtc.next());
            while(p < end)
              printf("%08x ",*p++);
            printf("\n");
          }
        }

        unsigned ntag = (in->datagram().env.value() >> 1)&0x1f;
        uint64_t pid  = in->datagram().seq.stamp().fiducials();
        
        if (_ntag >= 0 && int(ntag) != _ntag) {
          if (pid == _pid) {
            _errors->addvalue(1,1); // repeatFrames++;
          }
          _errors->addvalue(((ntag-_ntag)&0x1f), 0);
          in->datagram().xtc.damage.increase(Damage::UserDefined);
        }

//         Xtc* xtc = &in->datagram().xtc;
//         if (xtc->contains.value() == _generic1DDataType.value()) {
//           const Generic1DDataType& data = *reinterpret_cast<const Generic1DDataType*>(xtc->payload());
//         }

        unsigned atags=0, atagerrs=0;
        _tag.analyze(in->datagram(), atags, atagerrs); 
        _stats ->addvalue(atags, 1);
        _errors->addvalue(atagerrs, 3);

        timespec t; clock_gettime(CLOCK_REALTIME,&t);
        _stats->addvalue(1, 0);
        _stats->time(ClockTime(t));
        _errors->time(ClockTime(t));
#if 0
        return 0;
#endif
        return in;
      }
    private:
      AnaTag&  _tag;
      int      _ntag;
      uint64_t _pid;
      MonEntryScalar* _stats;
      MonEntryScalar* _errors;
    };

    class AllocAction : public Action {
    public:
      AllocAction(CfgClientNfs& cfg,
                  AnaTag&       tag,
                  ProcInfo&     info) : _cfg(cfg), _tag(tag), _info(info) {}
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _tag.allocate  (alloc.allocation());
        _cfg.initialize(alloc.allocation());
        return tr;
      }
      InDatagram* fire(InDatagram* dg) {
        _info = static_cast<ProcInfo&>(dg->xtc.src);
        return dg;
      }
    private:
      CfgClientNfs& _cfg;
      AnaTag&       _tag;
      ProcInfo&     _info;
    };

    class UnmapAction : public Action {
    public:
      UnmapAction(AnaTag& tag) : _tag(tag) {}
      Transition* fire(Transition* tr) {
        _tag.deallocate();
        return tr;
      }
    private:
      AnaTag& _tag;
    };

    class ConfigAction : public Action {
      enum {MaxConfigSize=0x1000};
    public:
      ConfigAction(TprDS::TprReg& dev,
                   const Src&     src,
                   Manager&       mgr,
                   CfgClientNfs&  cfg,
                   bool           lmon) :
        _dev    (dev),
        _config (new char[MaxConfigSize]),
        _cfgtc  (_generic1DConfigType,src),
        _mgr    (mgr),
        _cfg    (cfg),
        _lmon   (lmon),
        _occPool(sizeof(UserMessage),1) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) 
      {
        dg->insert(_cfgtc, _config);
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
                                  _tprdsConfigType,
                                  _config,
                                  sizeof(TprDSConfigType) );
        
        if (len) {
          TprDSConfigType& c = *reinterpret_cast<TprDSConfigType*>(_config);
          nsamples[0] = c.dataSize();
          _dev.base.dmaFullThr=c.fullThreshold(); 
          _dev.base.dmaHistEna(_lmon);
        }
        else {
          nsamples[0] = 20;
          _dev.base.dmaFullThr=512;
        }
        
        Generic1DConfigType& cfg = *new (_config) Generic1DConfigType(1,nsamples,sampletype,offset,period);
        _cfgtc.extent     = sizeof(Xtc) + cfg._sizeof();

        _dev.tpr.rxPolarity (false);
        _dev.tpr.resetCounts();
        _dev.dma.setEmptyThr(2);
          
        _dev.base.resetCounts();

        _dev.base.setupDaq(0,0,nsamples[0]);
        
        _dev.base.dump();
        _dev.dma .dump();
        _dev.tpr .dump();
        
        printf("Configuration Done\n");
        
        _nerror = 0;  // override
        
        if (_nerror) {
          UserMessage* msg = new (&_occPool) UserMessage;
          msg->append("TprDS: failed to apply configuration.\n");
          _mgr.appliance().post(msg);
        }
        else {
          RegisterPayload* msg = new (&_occPool) 
            RegisterPayload(cfg.data_offset(-1)+sizeof(Dgram)+sizeof(Xtc));
          _mgr.appliance().post(msg);
        }
        return tr;
      }
    private:
      TprDS::TprReg&      _dev;
      char*               _config;
      Xtc                 _cfgtc;
      Manager&            _mgr;
      CfgClientNfs&       _cfg;
      bool                _lmon;
      GenericPool         _occPool;
      unsigned            _nerror;
    };
    class BeginRun : public Action {
    public:
      BeginRun(StatsTimer& t,
               AnaTag& tag,
               const ProcInfo& info) : _t(t), _tag(tag), _info(info) {}
      ~BeginRun() {}
    public:
      InDatagram* fire(InDatagram* dg) { return dg; }
      Transition* fire(Transition* tr) {
        int offset = static_cast<const RunInfo*>(tr)->offset(_info);
        printf("BeginRun::offset is %d  [%x.%x]\n",
               offset, _info.log(), _info.phy());

        _tag.dump();
        nPrint=10;
        _t.start();
        return tr;
      }
    private:
      StatsTimer& _t;
      AnaTag& _tag;
      const ProcInfo& _info;
    };
    class EndRun : public Action {
    public:
      EndRun(StatsTimer& t,
             AnaTag& tag) : _t(t), _tag(tag) {}
      ~EndRun() {}
    public:
      InDatagram* fire(InDatagram* dg) { return dg; }
      Transition* fire(Transition* tr) {
        _t.cancel();
        _tag.dump();
        return tr;
      }
    private:
      StatsTimer& _t;
      AnaTag& _tag;
    };
    class UnconfigAction : public Action {
    public:
      UnconfigAction(TprDS::TprReg& dev) :
        _dev    (dev) {}
      ~UnconfigAction() {}
      InDatagram* fire(InDatagram* dg) 
      { return dg; }
      Transition* fire(Transition* tr)
      {
        _dev.base.disable(0);
        return tr;
      }
    private:
      TprDS::TprReg&      _dev;
    };
  }
}

using namespace Pds::TprDS;

StatsTimer::StatsTimer(TprReg&   dev,
                       MonGroup& group) :
  _dev      (dev),
  _task     (new Task(TaskObject("PtnS")))
{
  { std::vector<std::string> names;
    names.push_back("Events");
    names.push_back("Pauses");
    names.push_back("Overflows");
    //    names.push_back("Idles");
    names.push_back("Frames");
    MonDescScalar desc("DmaStats",names);
    _stats = new MonEntryScalar(desc);
    group.add(_stats); }
  { std::vector<std::string> names;
    names.push_back("Resets");
    names.push_back("DecErrs");
    names.push_back("DspErrs");
    MonDescScalar desc("RxLink",names);
    _rxlink = new MonEntryScalar(desc);
    group.add(_rxlink); }
  { std::vector<std::string> names;
    names.push_back("BuffFree");
    MonDescScalar desc("Dma",names);
    _dma = new MonEntryScalar(desc);
    group.add(_dma); }
}

void StatsTimer::expired()
{
  timespec t; clock_gettime(CLOCK_REALTIME,&t);
#define INCSTAT(b,v,i) { _stats->addvalue(b-_s.v,i); _s.v=b; }
  INCSTAT(_dev.base.channel[0].evtCount,evtCount,0);
  INCSTAT(_dev.base.pauseCount         ,pauseCount,1);
  INCSTAT(_dev.base.overflowCount      ,overflowCount,2);
  INCSTAT(_dev.base.frameCount         ,frameCount,3);
#undef INCSTAT
#define INCSTAT(b,v,i) { _rxlink->addvalue(b-_s.v,i); _s.v=b; }
  INCSTAT(_dev.tpr.RxRstDone,rxRstDone,0);
  INCSTAT(_dev.tpr.RxDecErrs,rxDecErrs,1);
  INCSTAT(_dev.tpr.RxDspErrs,rxDspErrs,2);
#undef INCSTAT
  _dma->addvalue(_dev.dma.rxFreeStat&0x3ff,0);
  ClockTime ct(t);
  _stats ->time(ct);
  _rxlink->time(ct);
  _dma   ->time(ct);
}

Manager::Manager(TprReg&       dev,
                 Server&       server,
                 CfgClientNfs& cfg,
                 bool          lmonitor) : _fsm(*new Fsm())
{
  MonGroup* group = new MonGroup("TprDS");
  VmonServerManager::instance()->cds().add(group);
  StatsTimer* stats = new StatsTimer(dev, *group);
  AnaTag*     tags  = new AnaTag(dev);
  ProcInfo*   info  = new ProcInfo(Level::Segment,0,0);

  _fsm.callback(TransitionId::Map      ,new AllocAction (cfg,*tags,*info));
  _fsm.callback(TransitionId::Configure,new ConfigAction(dev, server.client(), *this, cfg, lmonitor));
  _fsm.callback(TransitionId::BeginRun ,new BeginRun     (*stats,*tags,*info));
  _fsm.callback(TransitionId::L1Accept ,new L1Action     (*group,*tags));
  _fsm.callback(TransitionId::EndRun   ,new EndRun       (*stats,*tags));
  _fsm.callback(TransitionId::Unconfigure,new UnconfigAction (dev));
  _fsm.callback(TransitionId::Unmap    ,new UnmapAction  (*tags));
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

