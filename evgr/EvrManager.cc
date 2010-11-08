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
#include "pds/utility/Mtu.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/utility/ToNetEb.hh"
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
static const int      giMaxNumFifoEvent = 32;
static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 16; 
static const int      giMaxCalibCycles  = 500;
class L1Xmitter;
static L1Xmitter *l1xmitGlobal;
static EvgrBoardInfo < Evr > *erInfoGlobal; // yuck

/* 
 * Total Number of event code types is 256 ( Range: 0 - 255 )
 * 
 * Note: Used to initialize the EventCodeState list
 */
static const unsigned int guNumTypeEventCode = 256; 
struct EventCodeState
{
  bool bReadout;
  bool bTerminator;
  int  iDefReportDelay;
  int  iDefReportWidth;  
  int  iReportDelay;
  int  iReportWidth;
};

class L1Xmitter {
public:
  L1Xmitter(Evr & er, DoneTimer & done):
    _er                 (er),
    _done               (done),
    _outlet             (sizeof(EvrDatagram), 0, Ins   (Route::interface())),
    _evtCounter         (0), 
    _evtStop            (0), 
    _enabled            (false), 
    _lastfid            (0),
    _bReadout           (false),
    _pEvrConfig         (NULL),
    _L1DataUpdated      ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _L1DataFinal        ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _L1DataLatch        ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _bEvrDataFullUpdated(false),
    _bEvrDataFullFinal  (false)
  {
    new (&_L1DataUpdated) EvrDataUtil( 0, NULL );
    new (&_L1DataFinal)   EvrDataUtil( 0, NULL );
    new (&_L1DataLatch)   EvrDataUtil( 0, NULL );    
    
    memset( _lEventCodeState, 0, sizeof(_lEventCodeState) );
  }
  
  ~L1Xmitter()
  {
    delete (char*) &_L1DataUpdated;
    delete (char*) &_L1DataFinal;
    delete (char*) &_L1DataLatch;    
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

    // If Fifo is empty, an invalid event code will be returned
    if ( fe.EventCode >= guNumTypeEventCode )
      return;
    
    uint32_t uEventCode      = fe.EventCode;
    bool     bStartL1Accept  = false;
    
    /*
     * Determine if we need to start L1Accept
     *
     * Rule: If we have got an readout event before, and get the terminator event now,
     *    then we will start L1Accept 
     */

    EventCodeState& codeState = _lEventCodeState[uEventCode];
    
    if ( codeState.iDefReportWidth == 0 )// not an interesting event
    {
      if ( uEventCode == (unsigned) EvrManager::EVENT_CODE_BEAM  || // special event: Beam present -> always included in the report
           uEventCode == (unsigned) EvrManager::EVENT_CODE_BYKIK    // special event: Dark frame   -> always included in the report
         )
        addSpecialEvent( *(const EvrDataType::FIFOEvent*) &fe );
        
      return;
    }

    
    if ( codeState.iDefReportDelay == 0 )
      addFifoEventCheck( _L1DataUpdated, *(const EvrDataType::FIFOEvent*) &fe );
      
    if ( codeState.iDefReportDelay != 0 || codeState.iDefReportWidth > 1 )
    {
      _L1DataLatch.updateFifoEvent( *(const EvrDataType::FIFOEvent*) &fe );
      codeState.iReportDelay = codeState.iDefReportDelay;
      codeState.iReportWidth = codeState.iDefReportWidth;
    }
             
    if ( codeState.iDefReportDelay == 0 )
    {
      if ( codeState.bReadout )
        _bReadout = true;
      
      if ( codeState.bTerminator && _bReadout )
      {
        _bReadout       = false; // clear the readout flag
        bStartL1Accept  = true;
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
            
      if ( _L1DataFinal.numFifoEvents() != 0 )
      {
        printf( "L1Xmitter::xmit(): Previous Evr Data has not been transferred out. Current data will be reported in next round.\n" );
      }
      else
      {
        new (&_L1DataFinal) EvrDataType( 0, NULL );

        //// !! debug output
        //printf( "Latch data before processing:\n" );
        //_L1DataLatch.printFifoEvents();          
        
        /* 
         * Process delayed & enlongated events
         */
        bool bAnyLactchedEventDeleted = false;
        for (unsigned int uEventIndex = 0; uEventIndex < _L1DataLatch.numFifoEvents(); uEventIndex++ )
        {
          const EvrDataType::FIFOEvent& fifoEvent = _L1DataLatch.fifoEvent( uEventIndex );
          EventCodeState&               codeState = _lEventCodeState[fifoEvent.EventCode];
                                           
          //// !! debug output
          //printf( "Latch event code %u  report delay %u width %u\n", 
          //  fifoEvent.EventCode, codeState.iReportDelay, codeState.iReportWidth );
            
          if ( codeState.iReportDelay > 0 )
          {
            --codeState.iReportDelay;
            continue;
          }
          
          /*
           * Add the lacted events to final buffer, if this event has not been added before,
           * based on the following criterion:
           *   - If the report delay is 0, and 
           *   - this is the first report (i.e. report width == default report width)
           *       - then this event must have been added
           */
          if ( codeState.iDefReportDelay != 0 || codeState.iReportWidth != codeState.iDefReportWidth)
            addFifoEventCheck( _L1DataFinal, fifoEvent );
          --codeState.iReportWidth;
            
          if ( codeState.iReportWidth <= 0 )
          {
            _L1DataLatch.markEventAsDeleted( uEventIndex );
            bAnyLactchedEventDeleted = true;
          }
        }
        
        if ( bAnyLactchedEventDeleted )
          _L1DataLatch.PurgeDeletedEvents();          

        //// !! debug output
        //printf( "\nLatch data after processing:\n" );
        //_L1DataLatch.printFifoEvents();
        //printf( "\nFinal data after processing:\n" );
        //_L1DataFinal.printFifoEvents();

        for (unsigned int uEventIndex = 0; uEventIndex < _L1DataUpdated.numFifoEvents(); uEventIndex++ )
        {
          const EvrDataType::FIFOEvent& fifoEvent =  _L1DataUpdated.fifoEvent( uEventIndex );
          addFifoEventCheck( _L1DataFinal, fifoEvent );
        }
        
        _bEvrDataFullFinal = _bEvrDataFullUpdated;
        
        _L1DataUpdated.clearFifoEvents();
        _bEvrDataFullUpdated = false;

        //// !! debug output
        //printf( "\nFinal data after appending all data:\n" );
        //_L1DataFinal.printFifoEvents();        
        //printf( "\n" );
      } // if ( _L1DataFinal.numFifoEvents() == 0 )
      
      static const int NEVENTPRINT = 1000;      
      if (_evtCounter%NEVENTPRINT == 0) 
      {
        float dfid = (_lastfid < fe.TimestampHigh) ? 
	  fe.TimestampHigh-_lastfid :
	  fe.TimestampHigh+Pds::TimeStamp::MaxFiducials-_lastfid;
        float period=dfid/(float)(NEVENTPRINT)/360.0;
        float rate=0.0;
        if (period>1.e-8) rate=1./period;
        printf("Evr event %d, high/low 0x%x/0x%x, rate(Hz): %7.2f\n",
          _evtCounter, fe.TimestampHigh, fe.TimestampLow, rate);
        _lastfid = fe.TimestampHigh;
      }

      if (_evtCounter == _evtStop)
        _done.expired();                     
        
      /*
       * Send out the L1 "trigger" to make all L1 node start processing.
       *
       * Note: As soon as the send() function is called, the other polling thread may
       *   race with this thread to get the evr data and send it out immediately.
       */
      _outlet.send((char *) &datagram, 0, 0, _dst);      
    } // if ( bStartL1Accept )       
  } 

  void reset()        
  { 
    _evtCounter = 0; 
    _L1DataUpdated.clearFifoEvents();
    _L1DataFinal  .clearFifoEvents();
    _L1DataLatch  .clearFifoEvents();
    _bEvrDataFullUpdated = false;
    _bEvrDataFullFinal   = false;
  }
  void enable(bool e) { _enabled    = e; }
  bool enable() const { return _enabled; }
  
  void dst(const Ins & ins)  { _dst = ins; }
  
  void stopAfter(unsigned n)  { _evtStop = _evtCounter + n; }
  
  void setEvrConfig(const EvrConfigType* pEvrConfig) 
  { 
    _pEvrConfig = pEvrConfig; 
    
    memset( _lEventCodeState, 0, sizeof(_lEventCodeState) );

    unsigned int uEventIndex = 0; 
    for ( ; uEventIndex < _pEvrConfig->neventcodes(); uEventIndex++ )
    {
      const EvrConfigType::EventCodeType& eventCode = _pEvrConfig->eventcode( uEventIndex );
      if ( eventCode.code() >= guNumTypeEventCode )
      {
        printf( "L1Xmitter::setEvrConfig(): event code out of range: %d\n", eventCode.code() );
        continue;
      }
      
      EventCodeState& codeState = _lEventCodeState[eventCode.code()];
      codeState.bReadout        = eventCode.isReadout   ();
      codeState.bTerminator     = eventCode.isTerminator();
      codeState.iDefReportDelay = eventCode.reportDelay ();
      codeState.iDefReportWidth = eventCode.reportWidth ();             
    }    
  }
  
  const EvrDataUtil&  getL1Data     () { return _L1DataFinal; }
  bool                getL1DataFull () { return _bEvrDataFullFinal; }
  void                releaseL1Data () { _L1DataFinal.clearFifoEvents(); }
    
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
  EvrDataUtil&          _L1DataLatch;
  EventCodeState        _lEventCodeState[guNumTypeEventCode];
  bool                  _bEvrDataFullUpdated;
  bool                  _bEvrDataFullFinal;
  
  // Add Fifo event to the evrData with boundary check
  int addFifoEventCheck( EvrDataUtil& evrData, const EvrDataType::FIFOEvent& fe )
  {
    if ( (int) evrData.numFifoEvents() < giMaxNumFifoEvent )
      return evrData.addFifoEvent(fe);
    
    _bEvrDataFullUpdated = true; // set the flag and later the L1Accept will send out damage    
    return -1;
  }
  
  /*
   * Add a special event to the current evrData
   *
   *   Also checked the evrData content to remove all "previous" (not in the same timestamp) special events,
   *     until an "interesting" event is encountered.
   *
   * Note: This algorithm is based on the assumption that
   *   - All the "interesting" events occurs later than "special" events
   *      - Current settings: All "interesting" event codes (67 - 98) occurs after all special events (140, 162)
   */
  int addSpecialEvent( const EvrDataType::FIFOEvent &feCur )
  {
    uint32_t uFiducialCur  = feCur.TimestampHigh;
    
    for (int iEventIndex = _L1DataUpdated.numFifoEvents()-1; iEventIndex >= 0; iEventIndex-- )
    {
      const EvrDataType::FIFOEvent& fifoEvent  = _L1DataUpdated.fifoEvent( iEventIndex );
      uint32_t                      uFiducial  = fifoEvent.TimestampHigh;

      if ( uFiducial == uFiducialCur ) // still in the same timestamp. this event will be checked again in later iteration.
        break;
      
      uint32_t        uEventCode = fifoEvent.EventCode;
      EventCodeState& codeState  = _lEventCodeState[uEventCode];
    
      if ( codeState.iDefReportWidth != 0 ) // interesting event
        break;
        
      _L1DataUpdated.removeTailEvent();      
    }
    
    addFifoEventCheck( _L1DataUpdated, feCur );       
    return 0;
  }
};

class EvrAction:public Action
{
public:
  EvrAction(Evr & er):_er(er)
  {
  }
  Evr & _er;
};

class EvrL1Action:public EvrAction
{
public:
  EvrL1Action(Evr &      er, 
        const Src& src) :
    EvrAction(er),
    _iMaxEvrDataSize(sizeof(CDatagram) + sizeof(Xtc) + EvrDataUtil::size( giMaxNumFifoEvent )),
    _src            (src),
    _swtrig_out     (Route::interface(), Mtu::Size, 16),
    _poolEvrData    (_iMaxEvrDataSize, 16) {}

  void set_dst(const Ins& dst) { _swtrig_dst = dst; }

  InDatagram *fire(InDatagram * in)
  {
    InDatagram* out = in;

    do
    {
      if (!l1xmitGlobal->enable())
        break;

      if ( _poolEvrData.numberOfFreeObjects() <= 0 )
      {
        printf( "EvrL1Action::fire(): Pool is full, so cannot provide buffer for new datagram\n" );
        break;
      }      

      const EvrDataUtil& evrData   = l1xmitGlobal->getL1Data();
      bool               bDataFull = l1xmitGlobal->getL1DataFull();
      
      out = 
       new ( &_poolEvrData ) CDatagram( in->datagram() ); 
      out->datagram().xtc.alloc( sizeof(Xtc) + evrData.size() );
      
      if ( bDataFull )
      {
        printf( "EvrL1Action::fire(): Too many FIFO events recieved, so L1 data buffer is full.\n" );
        printf( "  Please check the readout and terminator event settings.\n" );        
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      }
      
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

      // !! debug print
      //if (  evrData.numFifoEvents() > 1 )
      //{
      //  printf( "Shot id = 0x%x\n", in->datagram().seq.stamp().fiducials() );
      //  printf( "EvrL1Action::fire() data dump start (size = %u bytes)\n", evrData.size() );
      //  evrData.printFifoEvents();
      //  printf( "EvrL1Action::fire() data dump end\n\n" );
      //}
      
      l1xmitGlobal->releaseL1Data();      
    } 
    while (false);
    
    //
    //  Special software trigger service to L1 nodes
    //
    {
      out->send(_swtrig_out, _swtrig_dst);
    }

    return out;
  }
  
private:
  const int   _iMaxEvrDataSize;
  Src         _src;
  ToNetEb     _swtrig_out;
  Ins         _swtrig_dst;
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

static unsigned int evrConfigSize(unsigned maxNumEventCodes, unsigned maxNumPulses, unsigned maxNumOutputMaps)
{
  return (sizeof(EvrConfigType) + 
    maxNumEventCodes * sizeof(EvrConfigType::EventCodeType) +
    maxNumPulses     * sizeof(EvrConfigType::PulseType) + 
    maxNumOutputMaps * sizeof(EvrConfigType::OutputMapType));
}

class EvrConfigManager
{
public:
  EvrConfigManager(Evr & er, CfgClientNfs & cfg, bool bTurnOffBeamCodes):
    _er (er),
    _cfg(cfg),
    _cfgtc(_evrConfigType, cfg.src()),
    _configBuffer(
      new char[ giMaxCalibCycles* evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] ),
    _cur_config(0),
    _end_config(0),
    _bTurnOffBeamCodes(_bTurnOffBeamCodes)
  {
  }

   ~EvrConfigManager()
  {
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

    const EvrConfigType& cfg = *_cur_config;

    printf("Configuring evr\n");

    _er.Reset();

    // Problem in Reset() function: It doesn't reset the set and clear masks
    // workaround: manually call the clear function to set and clear all masks
    for (unsigned ram=0;ram<2;ram++) {
      for (unsigned iopcode=0;iopcode<=EVR_MAX_EVENT_CODE;iopcode++) {
        for (unsigned jSetClear=0;jSetClear<EVR_MAX_PULSES;jSetClear++)
          _er.ClearPulseMap(ram, iopcode, jSetClear, jSetClear, jSetClear);
      }
    }    

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
    
    if (!_bTurnOffBeamCodes)
    {
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BEAM,  enable);
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BYKIK, enable);
    }
    
    unsigned dummyram = 1;
    _er.MapRamEnable(dummyram, 1);
    
    l1xmitGlobal->setEvrConfig( &cfg );
  }

  bool fetch(Transition* tr)
  {
    _cfgtc.damage = 0;

    int len = _cfg.fetch(*tr, _evrConfigType, _configBuffer);
    if (len < 0)
    {
      _cur_config = 0;
      _cfgtc.extent = sizeof(Xtc);
      _cfgtc.damage.increase(Damage::UserDefined);
      printf("Config::configure failed to retrieve Evr configuration\n");
      return false;
    }

    _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
    _end_config = _configBuffer+len;
    _cfgtc.extent = sizeof(Xtc) + _cur_config->size();

    return true;
  }

  void advance()
  {
    int len = _cur_config->size();
    const char* nxt_config = reinterpret_cast<const char*>(_cur_config)+len;
    if (nxt_config < _end_config)
      _cur_config = reinterpret_cast<const EvrConfigType*>(nxt_config);
    else
      _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
    _cfgtc.extent = sizeof(Xtc) + _cur_config->size();
  }

  void enable()
  {
    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    _er.EnableFIFO(1);
    _er.Enable(1);
  }

  void disable()
  {
    _er.IrqEnable(0);
    _er.Enable(0);
    _er.EnableFIFO(0);
  }

private:
  Evr&            _er;
  CfgClientNfs&   _cfg;
  Xtc             _cfgtc;
  char *          _configBuffer;
  const EvrConfigType* _cur_config;
  const char*          _end_config;
  bool                 _bTurnOffBeamCodes;
};

class EvrConfigAction: public Action {
public:
  EvrConfigAction(EvrConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr) 
  { 
    _cfg.fetch(tr);
    l1xmitGlobal->reset();
    return tr;
  }
  InDatagram* fire(InDatagram* dg) { _cfg.insert(dg); return dg; }
private:
  EvrConfigManager& _cfg;
};

class EvrBeginCalibAction: public Action {
public:
  EvrBeginCalibAction(EvrConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr) { _cfg.configure(); _cfg.enable(); return tr; }
  InDatagram* fire(InDatagram* dg) { _cfg.insert(dg); return dg; }
private:
  EvrConfigManager& _cfg;
};

class EvrEndCalibAction: public Action
{
public:
  EvrEndCalibAction(EvrConfigManager& cfg): _cfg(cfg) {}
public:
  Transition *fire(Transition * tr) { _cfg.disable(); return tr; }
  InDatagram *fire(InDatagram * dg) { _cfg.advance(); return dg; }
private:
  EvrConfigManager& _cfg;
};


class EvrAllocAction:public Action
{
public:
  EvrAllocAction(CfgClientNfs & cfg, 
		 EvrL1Action&   l1A,
		 DoneTimer&     done) :
    _cfg(cfg),_l1action(l1A), _done(done), _xmitter(0)
  {
  }
  Transition *fire(Transition * tr)
  {
    const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
    _cfg.initialize(alloc.allocation());

    if (!_xmitter)
      _xmitter = new L1Xmitter(_l1action._er,_done);
    l1xmitGlobal = _xmitter;

    //
    //  Test if we own the primary EVR for this partition
    //
#if 1
    unsigned nnodes = alloc.allocation().nnodes();
    int pid = getpid();
    for (unsigned n = 0; n < nnodes; n++)
    {
      const Node *node = alloc.allocation().node(n);
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
    _l1action.set_dst(StreamPorts::event(alloc.allocation().partitionid(),
           Level::Observer));
    return tr;
  }
private:
  CfgClientNfs& _cfg;
  EvrL1Action&  _l1action;
  DoneTimer&    _done;
  L1Xmitter*    _xmitter;
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

EvrManager::EvrManager(EvgrBoardInfo < Evr > &erInfo, CfgClientNfs & cfg, bool bTurnOffBeamCodes):
  _er(erInfo.board()), _fsm(*new Fsm), _done(new DoneTimer(_fsm)), _bTurnOffBeamCodes(bTurnOffBeamCodes)
{
  EvrL1Action* l1A = new EvrL1Action(_er, cfg.src());
  EvrConfigManager* cmgr = new EvrConfigManager(_er, cfg, bTurnOffBeamCodes);

  _fsm.callback(TransitionId::Map            , new EvrAllocAction     (cfg,*l1A,*_done));
  _fsm.callback(TransitionId::Configure      , new EvrConfigAction    (*cmgr));
  _fsm.callback(TransitionId::BeginCalibCycle, new EvrBeginCalibAction(*cmgr));
  _fsm.callback(TransitionId::EndCalibCycle  , new EvrEndCalibAction  (*cmgr));
  _fsm.callback(TransitionId::Enable         , new EvrEnableAction    (_er, *_done));
  _fsm.callback(TransitionId::Disable        , new EvrDisableAction   (_er, *_done));
  _fsm.callback(TransitionId::L1Accept       , l1A);

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

const int EvrManager::EVENT_CODE_BEAM;  // value is defined in the header file
const int EvrManager::EVENT_CODE_BYKIK; // value is defined in the header file
