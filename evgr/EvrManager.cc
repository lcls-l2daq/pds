#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <new>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>

#include "EvgrBoardInfo.hh"
#include "EvrManager.hh"
#include "EvrDataUtil.hh"

#include "evgr/evr/evr.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/Route.hh"
#include "pds/service/Client.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types

#include "pds/service/Timer.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pds/xtc/CDatagram.hh"

namespace Pds
{
  class DoneTimer:public Timer
  {
  public:
    DoneTimer(Appliance & app):_app(app),
      _pool(sizeof(Occurrence), 1), _task(new Task(TaskObject("donet")))
    {
    }
     ~DoneTimer()
    {
      _task->destroy();
    }
  public:
    void set_duration_ms(unsigned v)
    {
      _duration = v;
    }
  public:
    void expired()
    {
      _app.post(new(&_pool) Occurrence(OccurrenceId::SequencerDone));
    }
    Task *task()
    {
      return _task;
    }
    unsigned duration() const
    {
      return _duration;
    }
    unsigned repetitive() const
    {
      return 0;
    }
  private:
    Appliance &           _app;
    GenericPool           _pool;
    Task *                _task;
    unsigned              _duration;
  };
};

using namespace Pds;

static unsigned dropPulseMask     = 0xffffffff;
static int      giMaxNumFifoEvent = 360;
static int      giMaxEventCodes   = 32;
static int      giMaxPulses       = 10;
static int      giMaxOutputMaps   = 16;

class L1Xmitter;
static L1Xmitter *l1xmitGlobal;
static EvgrBoardInfo < Evr > *erInfoGlobal; // yuck

class L1Xmitter {
public:
  L1Xmitter(Evr & er, DoneTimer & done):
    _er           (er),
    _done         (done),
    _outlet       (sizeof(EvrDatagram), 0, Ins   (Route::interface())),
    _evtCounter   (0), 
    _evtStop      (0), 
    _enabled      (false), 
    _lastfid      (0),
    _bReadout     (false),
    _pEvrConfig   (NULL),
    _L1DataUpdated( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _L1DataFinal  ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  )    
  {
    new (&_L1DataUpdated) EvrDataUtil( 0, NULL );
    new (&_L1DataFinal)   EvrDataUtil( 0, NULL );
  }
  
  ~L1Xmitter()
  {
    delete (char*) &_L1DataUpdated;
    delete (char*) &_L1DataFinal;
  }
  
  void xmit()
  {
    FIFOEvent fe;
    _er.GetFIFOEvent(&fe);

    // for testing
    if ((fe.TimestampHigh & dropPulseMask) == dropPulseMask)
    {
      printf("Dropping %x\n", fe.TimestampHigh);
      return;
    }
    
    uint8_t uEventCode      = fe.EventCode;
    bool    bStartL1Accept  = false;
    
    /*
     * Determine if we need to start L1Accept
     *
     * Rule: If we got an readout event before, and get the terminator event now,
     *    then we will start L1Accept 
     */
    if ( _pEvrConfig != NULL )
    {
      for ( unsigned int uEventIndex = 0; uEventIndex < _pEvrConfig->neventcodes(); uEventIndex++ )
      {
        const EvrConfigType::EventCodeType& eventCode = _pEvrConfig->eventcode( uEventIndex );
        if ( eventCode.code() != uEventCode )
          continue;
          
        if ( eventCode.isReadout() )
          _bReadout = true;
        
        if ( eventCode.isTerminator() && _bReadout )
        {
          _bReadout       = false; // clear the readout flag
          bStartL1Accept  = true;
        }        
        
        _L1DataUpdated.addFifoEvent( *(const EvrData::DataV3::FIFOEvent*) &fe );
        
        break; // skip all the remaining event codes
      }
    }

    if ( bStartL1Accept )
    {      
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ClockTime ctime(ts.tv_sec, ts.tv_nsec);
      TimeStamp stamp(fe.TimestampLow, fe.TimestampHigh, _evtCounter);
      Sequence seq(Sequence::Event, TransitionId::L1Accept, ctime, stamp);
      EvrDatagram datagram(seq, _evtCounter++);
      
      _outlet.send((char *) &datagram, 0, 0, _dst);
      
      if ( _L1DataFinal.numFifoEvents() == 0 )
        new (&_L1DataFinal) EvrData::DataV3( _L1DataUpdated );
      else
      {
        printf( "L1Xmitter::xmit(): Previous Evr Data has not been transferred out. Current data will be skipped.\n" );
      }

      _L1DataUpdated.clearFifoEvents();      
      
      static const int NEVENTPRINT = 100;      
      if (_evtCounter%NEVENTPRINT == 0) 
      {
        float dfid = fe.TimestampHigh-_lastfid;
        if (fe.TimestampHigh<_lastfid) dfid += float(Pds::TimeStamp::MaxFiducials);
        float period=dfid/(float)(NEVENTPRINT)/360.0;
        float rate=0.0;
        if (period>1.e-8) rate=1./period;
        printf("Evr event %d, high/low 0x%x/0x%x, rate(Hz): %7.2f\n",
          _evtCounter, fe.TimestampHigh, fe.TimestampLow, rate);
        _lastfid = fe.TimestampHigh;
      }

      //static unsigned   nl1 = 0;
      //static Sequence   evrseq_fifo[4];      
      //  if (evrseq_fifo[nl1&3].stamp().fiducials()==seq.stamp().fiducials()) {
      //    unsigned i=nl1-3;
      //    do {
      //const Sequence& s = evrseq_fifo[i&3];
      //printf("=== fid %x: clk %x/%x\n",
      //       s.stamp().fiducials(),
      //       s.clock().seconds(),
      //       s.clock().nanoseconds());
      //    } while( i++!=nl1 );
      //    printf("*** fid %x: clk %x/%x: evtcode %x\n",
      //     seq.stamp().fiducials(),
      //     seq.clock().seconds(),
      //     seq.clock().nanoseconds(),
      //     fe.EventCode);
      //  }
      //  evrseq_fifo[++nl1&3] = seq;
      
      if (_evtCounter == _evtStop)
        _done.expired();                     
    }
  }
  
  void reset()  { _evtCounter = 0; }
  
  void enable(bool e) { _enabled = e; }
  bool enable() const { return _enabled; }
  
  void dst(const Ins & ins)  { _dst = ins; }
  
  void stopAfter(unsigned n)  { _evtStop = _evtCounter + n; }
  
  void setEvrConfig(const EvrConfigType* pEvrConfig) { _pEvrConfig = pEvrConfig; }
  
  EvrDataUtil& getL1Data() { return _L1DataFinal; }
  
private:
  Evr &                 _er;
  DoneTimer &           _done;
  Client                _outlet;
  Ins                   _dst;
  unsigned              _evtCounter;
  unsigned              _evtStop;
  bool                  _enabled;
  unsigned              _lastfid;
  bool                  _bReadout;
  const EvrConfigType*  _pEvrConfig;
  EvrDataUtil&          _L1DataUpdated;
  EvrDataUtil&          _L1DataFinal;
};

class EvrAction:public Action
{
protected:
  EvrAction(Evr & er):_er(er)
  {
  }
  Evr & _er;
};

class EvrL1Action:public EvrAction
{
public:
  EvrL1Action(Evr & er, const Src& src) : EvrAction(er),
    _iMaxEvrDataSize(sizeof(Xtc) + EvrDataUtil::size( giMaxNumFifoEvent )),
    _src            (src),
    _poolEvrData    (_iMaxEvrDataSize, 16)
  {
  }
  InDatagram *fire(InDatagram * in)
  {
    InDatagram* out = in;
        
    if (l1xmitGlobal->enable())
    {
      EvrDataUtil& evrData = l1xmitGlobal->getL1Data();
      
      if ( _poolEvrData.numberOfFreeObjects() <= 0 )
      {
        printf( "EvrL1Action::fire(): Pool is full, so cannot provide buffer for new datagram\n" );
        return NULL;
      }      
      
      out = 
       new ( &_poolEvrData ) CDatagram( in->datagram() ); 
      out->datagram().xtc.alloc( sizeof(Xtc) + evrData.size() );
      
      /*
       * Set Evr object
       */    
      char* pcXtc = (char*) out + sizeof(CDatagram);        
      TypeId typeEvrData(TypeId::Id_EvrData, EvrDataUtil::Version);
      Xtc* pXtc = 
       new (pcXtc) Xtc(typeEvrData, _src);
      pXtc->alloc( evrData.size() );   

      char*  pcEvrData = (char*) (pXtc + 1);  
      new (pcEvrData) EvrDataUtil(evrData);
      
      //printf( "EvrL1Action::fire() data dump start (size = %u bytes)\n", evrData.size() );
      //evrData.printFifoEvents();
      //printf( "EvrL1Action::fire() data dump end\n\n" );
      
      evrData.clearFifoEvents();
    }
    
    return out;
  }
  
private:
  const int   _iMaxEvrDataSize;
  Src         _src;
  GenericPool _poolEvrData;
};

class EvrEnableAction:public EvrAction
{
public:
  EvrEnableAction(Evr& er, DoneTimer& done) :
    EvrAction(er),
    _done    (done) {}
    
  InDatagram* fire(InDatagram* tr) 
  {
    if (l1xmitGlobal->enable()) 
    {
      const EnableEnv& env = static_cast<const EnableEnv&>(tr->datagram().env);
      l1xmitGlobal->stopAfter(env.events());
      if (env.timer()) 
      {
        _done.set_duration_ms(env.duration());
        _done.start();
      }
    }

    unsigned ram = 0;
    _er.MapRamEnable(ram, 1);
    return tr;
  }
private:
  DoneTimer & _done;
};

class EvrDisableAction:public EvrAction
{
public:
  EvrDisableAction(Evr& er, DoneTimer& done) : 
    EvrAction(er), _done(done) {}
    
  Transition* fire(Transition* tr) 
  {
    // switch to the "dummy" map ram so we can still get eventcodes 0x70,0x71,0x7d for timestamps
    unsigned dummyram=1;
    _er.MapRamEnable(dummyram,1);

    _done.cancel();
    return tr;
  }
private:
  DoneTimer & _done;
};

class EvrBeginRunAction:public EvrAction
{
public:
  EvrBeginRunAction(Evr & er):EvrAction(er)
  {
  }
  Transition *fire(Transition * tr)
  {
    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    _er.EnableFIFO(1);
    _er.Enable(1);
    return tr;
  }
};

class EvrEndRunAction:public EvrAction
{
public:
  EvrEndRunAction(Evr & er):EvrAction(er)
  {
  }
  Transition *fire(Transition * tr)
  {
    _er.IrqEnable(0);
    _er.Enable(0);
    _er.EnableFIFO(0);
    return tr;
  }
};

static unsigned int evrConfigSize(unsigned maxNumEventCodes, unsigned maxNumPulses, unsigned maxNumOutputMaps)
{
  return (sizeof(EvrConfigType) + 
    maxNumEventCodes * sizeof(EvrConfigType::EventCodeType) +
    maxNumPulses     * sizeof(EvrConfigType::PulseType) + 
    maxNumOutputMaps * sizeof(EvrConfigType::OutputMapType));
}

class EvrConfigAction:public EvrAction
{
public:
  EvrConfigAction(Evr & er,
      CfgClientNfs & cfg):EvrAction(er),
    _cfg(cfg),
    _cfgtc(_evrConfigType, cfg.src()),
    _configBuffer(
      new char[ evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] )
  {
  }

   ~EvrConfigAction()
  {
    delete[] _configBuffer;
  }

  InDatagram *fire(InDatagram * dg)
  {
    dg->insert(_cfgtc, _configBuffer);
    return dg;
  }

  Transition *fire(Transition * tr)
  {
    _cfgtc.damage.increase(Damage::UserDefined);
    int len = _cfg.fetch(*tr, _evrConfigType, _configBuffer);
    if (len < 0)
    {
      printf("Config::configure failed to retrieve Evr configuration\n");
      return tr;
    }
    const EvrConfigType & cfg = * (EvrConfigType*) _configBuffer;
    _cfgtc.extent = sizeof(Xtc) + cfg.size();

    printf("Configuring evr\n");

    _er.Reset();

    // setup map ram
    int ram = 0;
    int enable = l1xmitGlobal->enable();

    for (unsigned k = 0; k < cfg.npulses(); k++)
    {
      const EvrConfigType::PulseType & pc = cfg.pulse(k);
      _er.SetPulseProperties(
        pc.pulseId(),
        pc.polarity(),
        1, // Enable reset from event code
        1, // Enable set from event code
        1, // Enable trigger from event code        
        1  // Enable pulse
        );
        
      _er.SetPulseParams(pc.pulseId(), pc.prescale(), pc.delay(), pc.width());
    }

    for (unsigned k = 0; k < cfg.noutputs(); k++)
    {
      const EvrConfigType::OutputMapType & map = cfg.output_map(k);
      switch (map.conn())
      {
      case EvrConfigType::OutputMapType::FrontPanel:
        _er.SetFPOutMap(map.conn_id(), map.map());
        break;
      case EvrConfigType::OutputMapType::UnivIO:
        _er.SetUnivOutMap(map.conn_id(), map.map());
        break;
      }
    }
    
    /*
     * enable event codes, and setup each event code's pulse mapping
     */
    for (unsigned int uEventIndex = 0; uEventIndex < cfg.neventcodes(); uEventIndex++ )
    {
      const EvrConfigType::EventCodeType& eventCode = cfg.eventcode(uEventIndex);
              
      _er.SetFIFOEvent(ram, eventCode.code(), enable);
      
      unsigned int  uPulseBit     = 0x0001;
      uint32_t      u32TotalMask  = ( eventCode.maskTrigger() | eventCode.maskSet() | eventCode.maskClear() );
      
      for ( int iPulseIndex = 0; iPulseIndex < EVR_MAX_PULSES; iPulseIndex++, uPulseBit <<= 1 )
      {
        if ( (u32TotalMask & uPulseBit) == 0 ) continue;
        
        _er.SetPulseMap(ram, eventCode.code(), 
          ((eventCode.maskTrigger() & uPulseBit) != 0 ? iPulseIndex : -1 ),
          ((eventCode.maskSet()     & uPulseBit) != 0 ? iPulseIndex : -1 ),
          ((eventCode.maskClear()   & uPulseBit) != 0 ? iPulseIndex : -1 )
          );          
      }
    }
    
    _er.MapRamEnable(ram, 0);
    
    l1xmitGlobal->reset();
    l1xmitGlobal->setEvrConfig( &cfg );

    _cfgtc.damage = 0;
    return tr;
  }

private:
  CfgClientNfs & _cfg;
  Xtc _cfgtc;
  char *_configBuffer;
};

class EvrAllocAction:public Action
{
public:
  EvrAllocAction(CfgClientNfs & cfg):_cfg(cfg)
  {
  }
  Transition *fire(Transition * tr)
  {
    const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
    _cfg.initialize(alloc.allocation());
    //
    //  Test if we own the primary EVR for this partition
    //
#if 0
    unsigned nnodes = alloc.nnodes();
    int pid = getpid();
    for (unsigned n = 0; n < nnodes; n++)
    {
      const Node *node = alloc.node(n);
      if (node->pid() == pid)
      {
        printf("Found our EVR\n");
        l1xmitGlobal->enable(true);
        break;
      }
      else if (node->level() == Level::Segment)
      {
        printf("Found other EVR\n");
        l1xmitGlobal->enable(false);
        break;
      }
    }
#else
    l1xmitGlobal->enable(true);
#endif
    l1xmitGlobal->dst(StreamPorts::event(alloc.allocation().partitionid(),
      Level::Segment));
    return tr;
  }
private:
  CfgClientNfs & _cfg;
};

extern "C"
{
  void evrmgr_sig_handler(int parm)
  {
    Evr & er = erInfoGlobal->board();
    int flags = er.GetIrqFlags();
    if (flags & EVR_IRQFLAG_EVENT)
    {
      er.ClearIrqFlags(EVR_IRQFLAG_EVENT);
      l1xmitGlobal->xmit();
    }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance & EvrManager::appliance()
{
  return _fsm;
}

EvrManager::EvrManager(EvgrBoardInfo < Evr > &erInfo, CfgClientNfs & cfg):
_er(erInfo.board()), _fsm(*new Fsm), _done(new DoneTimer(_fsm))
{

  l1xmitGlobal = new L1Xmitter(_er, *_done);

  _fsm.callback(TransitionId::Map, new EvrAllocAction(cfg));
  _fsm.callback(TransitionId::Configure, new EvrConfigAction(_er, cfg));
  _fsm.callback(TransitionId::BeginRun, new EvrBeginRunAction(_er));
  _fsm.callback(TransitionId::EndRun, new EvrEndRunAction(_er));
  _fsm.callback(TransitionId::Enable, new EvrEnableAction(_er, *_done));
  _fsm.callback(TransitionId::Disable, new EvrDisableAction(_er, *_done));
  _fsm.callback(TransitionId::L1Accept, new EvrL1Action(_er, cfg.src()));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = EvrManager::sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

  if (sigaction(SIGINT, &int_action, 0) > 0)
  {
    printf("Couldn't set up SIGINT handler\n");
  }
}

EvrManager::~EvrManager()
{
  delete _done;
}

void EvrManager::drop_pulses(unsigned id)
{
  dropPulseMask = id;
}

void EvrManager::sigintHandler(int)
{
  printf("SIGINT received\n");

  if (erInfoGlobal && erInfoGlobal->mapped())
  {
    Evr & er = erInfoGlobal->board();
    printf("Stopping triggers and multicast... ");
    // switch to the "dummy" map ram so we can still get eventcodes 0x70,0x71,0x7d for timestamps
    unsigned dummyram=1;
    er.MapRamEnable(dummyram,1);
    printf("stopped\n");
  }
  else
  {
    printf("evr not mapped\n");
  }
  exit(0);
}
