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
#include "EvrCfgClient.hh"
#include "EvrL1Data.hh"

#include "evgr/evr/evr.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/Route.hh"
#include "pds/service/Client.hh"
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

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TimeStamp.hh"

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

static unsigned       dropPulseMask     = 0xffffffff;
static const int      giMaxNumFifoEvent = 32;
static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giNumL1Buffers    = 32; // number of L1 data buffers
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 16; 
static const int      giMaxCalibCycles  = 500;
static const unsigned giMaxCommands     = 32;
static const unsigned TERMINATOR        = 1;
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
  bool bCommand;
  int  iDefReportDelay;
  int  iDefReportWidth;  
  int  iReportWidth;
  int  iReportDelayQ; // First-order  delay for Control-Transient events
  int  iReportDelay;  // Second-order delay for Control-Transient events; First-order delay for Control-Latch events
};

/*
 * Forward declaration
 *  
 * bool evrHasEvent(): check if evr has any unprocessed fifo event
 */
//bool evrHasEvent(Evr& er);

/*
 * Signal handler, for processing the incoming event codes, and providing interfaces for
 *   retrieving L1 data from the L1Xmitter object
 */
class L1Xmitter {
public:
  L1Xmitter(Evr & er, DoneTimer & done):
    //uFiducialPrev       (0),
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
    _L1DataLatchQ       ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _L1DataLatch        ( *(EvrDataUtil*) new char[ EvrDataUtil::size( giMaxNumFifoEvent ) ]  ),
    _bEvrDataFullUpdated(false),
    _bEvrDataIncomplete (false),
    _ncommands          (0),
    _evrL1Data          (giMaxNumFifoEvent, giNumL1Buffers)
  {
    new (&_L1DataUpdated) EvrDataUtil( 0, NULL );
    new (&_L1DataLatch)   EvrDataUtil( 0, NULL );    
    new (&_L1DataLatchQ)  EvrDataUtil( 0, NULL );    
    
    memset( _lEventCodeState, 0, sizeof(_lEventCodeState) );
  }
  
  ~L1Xmitter()
  {
    delete[] (char*) &_L1DataUpdated;
    delete[] (char*) &_L1DataLatch;    
    delete[] (char*) &_L1DataLatchQ;
  }
  
  void xmit(const FIFOEvent& fe)
  {      
    /*
     * Determine if we need to start L1Accept
     *
     * Rule: If we have got an readout event before, and get the terminator event now,
     *    then we will start L1Accept 
     */
     
    if (fe.EventCode == TERMINATOR) {            
      // TERMINATOR will not be stored in the event code list
      if (_bReadout || _ncommands) 
      {
        _evrL1Data.setDataWriteIncomplete(false);
        startL1Accept(fe);
      }
      
      return;
    }

    uint32_t        uEventCode = fe.EventCode;
    EventCodeState& codeState  = _lEventCodeState[uEventCode];
    
    if ( codeState.iDefReportWidth == 0 )// not an interesting event
    {
      if ( uEventCode == (unsigned) EvrManager::EVENT_CODE_BEAM  || // special event: Beam present -> always included in the report
           uEventCode == (unsigned) EvrManager::EVENT_CODE_BYKIK    // special event: Dark frame   -> always included in the report
         )
        addSpecialEvent( _L1DataUpdated, *(const EvrDataType::FIFOEvent*) &fe );
        
      return;
    }
    
    if ( codeState.bReadout ) 
    {
      addFifoEventCheck( _L1DataUpdated, *(const EvrDataType::FIFOEvent*) &fe );      
      _lastFiducial = fe.TimestampHigh;
      _bReadout     = true;           
      return;
    }

    if ( codeState.bCommand )
    {
      _lastFiducial = fe.TimestampHigh;
      addCommand( fe );            
      return;
    }
    
    // for testing
    if ((fe.TimestampHigh & dropPulseMask) == dropPulseMask)
    {
      printf("Dropping %x\n", fe.TimestampHigh);
      return;
    }
    
    /*
     * The readout and command codes are processed above.
     * Now start to process the control-transient and control-latch codes
     */    

    /*
     *  Removed "latched" event codes if its partner ("release") code is found
     */
    { 
      bool eventDeleted = false;
      for (unsigned int uEventIndex = 0; uEventIndex < _L1DataLatchQ.numFifoEvents(); uEventIndex++ )
      {
        const EvrDataType::FIFOEvent& fifoEvent = _L1DataLatchQ.fifoEvent( uEventIndex );
        EventCodeState&               codeState = _lEventCodeState[fifoEvent.EventCode];
        if (codeState.iReportWidth == -int(uEventCode)) 
        {
          _L1DataLatchQ.markEventAsDeleted( uEventIndex );
          eventDeleted = true;
        }
      }
      for (unsigned int uEventIndex = 0; uEventIndex < _L1DataLatch.numFifoEvents(); uEventIndex++ )
      {
        const EvrDataType::FIFOEvent& fifoEvent = _L1DataLatch.fifoEvent( uEventIndex );
        EventCodeState&               codeState = _lEventCodeState[fifoEvent.EventCode];
        if (codeState.iReportWidth == -int(uEventCode)) 
        {
          _L1DataLatch.markEventAsDeleted( uEventIndex );
          eventDeleted = true;
        }
      }
        
      if ( eventDeleted )
      {
        _L1DataLatch .purgeDeletedEvents();          
        _L1DataLatchQ.purgeDeletedEvents();          
      }
    }
      
    if (codeState.iReportWidth > 0) {
      if (codeState.iReportDelayQ <= 0) {
        updateFifoEventCheck( _L1DataLatchQ, *(const EvrDataType::FIFOEvent*) &fe );
        codeState.iReportDelayQ = codeState.iDefReportDelay;
      }
      else {
        printf("L1Xmitter::xmit(): Can't queue eventcode %d\n",fe.EventCode);
      }
    }
    else {
      updateFifoEventCheck( _L1DataLatch, *(const EvrDataType::FIFOEvent*) &fe );
      codeState.iReportDelay = codeState.iDefReportDelay;
      codeState.iReportWidth = codeState.iDefReportWidth;
    }
  }

  void startL1Accept(const FIFOEvent& fe)
  {      
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ClockTime ctime(ts.tv_sec, ts.tv_nsec);
    TimeStamp stamp(fe.TimestampLow, fe.TimestampHigh, _evtCounter);

    if (!_bReadout) 
    {
      Sequence seq(Sequence::Occurrence, TransitionId::Unknown, ctime, stamp);
      EvrDatagram datagram(seq, _evtCounter, _ncommands);
      _outlet.send((char *) &datagram, _commands, _ncommands, _dst);      
    }
    else 
    {
      Sequence seq(Sequence::Event, TransitionId::L1Accept, ctime, stamp);
      EvrDatagram datagram(seq, _evtCounter++, _ncommands);
                  
      if ( ! _evrL1Data.isDataWriteReady() )
      {
        printf( "L1Xmitter::startL1Accept(): Previous Evr Data has not been transferred out.\n"
          "  Current data will be reported in next round.\n"
          "  Trigger counter: %d, Current Data counter: %d\n"
          "  Read Index = %d, Write Index = %d\n", 
          stamp.vector() ,        _evrL1Data.getCounterRead(),
          _evrL1Data.readIndex(), _evrL1Data.writeIndex() );
      }
      else
      {
        EvrDataUtil& _lL1DataFianl = _evrL1Data.getDataWrite();
        
        _evrL1Data.setCounterWrite(stamp.vector());

        /* 
         * Process delayed & enlongated events
         */
        bool bAnyLatchedEventDeleted = false;
        for (unsigned int uEventIndex = 0; uEventIndex < _L1DataLatchQ.numFifoEvents(); uEventIndex++ )
          {
            const EvrDataType::FIFOEvent& fifoEvent = _L1DataLatchQ.fifoEvent( uEventIndex );
            EventCodeState&               codeState = _lEventCodeState[fifoEvent.EventCode];
            
            if ( codeState.iReportWidth  <= 0 ||
                 codeState.iReportDelayQ <= 0 )
              {
                updateFifoEventCheck( _L1DataLatch, fifoEvent );
                _L1DataLatchQ.markEventAsDeleted( uEventIndex );
                bAnyLatchedEventDeleted = true;
                codeState.iReportDelay  = codeState.iReportDelayQ;
                codeState.iReportWidth  = codeState.iDefReportWidth;
                codeState.iReportDelayQ = 0;
                continue;
              }
            
            --codeState.iReportDelayQ;
          }

        if ( bAnyLatchedEventDeleted ) {
          _L1DataLatchQ.purgeDeletedEvents();     
          bAnyLatchedEventDeleted = false;
        }
          
        for (unsigned int uEventIndex = 0; uEventIndex < _L1DataLatch.numFifoEvents(); uEventIndex++ )
          {
            const EvrDataType::FIFOEvent& fifoEvent = _L1DataLatch.fifoEvent( uEventIndex );
            EventCodeState&               codeState = _lEventCodeState[fifoEvent.EventCode];
                                       
            if ( codeState.iReportDelay > 0 )
              {
                --codeState.iReportDelay;
                continue;
              }
      
            /*
             * Add the latched events to final buffer
             */
            addFifoEventCheck( _lL1DataFianl, fifoEvent );
            
            /*
             * Test if any control-transient event codes has expired.
             */
            if (codeState.iReportWidth>=0) // control-latch codes will bypass this test, since its report width = -1 * (release code)
              {
                if ( --codeState.iReportWidth <= 0 )
                  {
                    _L1DataLatch.markEventAsDeleted( uEventIndex );
                    bAnyLatchedEventDeleted = true;
                  }
              }
          }
    
        if ( bAnyLatchedEventDeleted ) {
          _L1DataLatch .purgeDeletedEvents();  
          bAnyLatchedEventDeleted = false;
        }
        
        for (unsigned int uEventIndex = 0; uEventIndex < _L1DataUpdated.numFifoEvents(); uEventIndex++ )
          {
            const EvrDataType::FIFOEvent& fifoEvent =  _L1DataUpdated.fifoEvent( uEventIndex );
            addFifoEventCheck( _lL1DataFianl, fifoEvent );
          }                    
        
        _evrL1Data.setDataWriteFull       ( _bEvrDataFullUpdated );
        _evrL1Data.setDataWriteIncomplete ( _bEvrDataIncomplete );
        _evrL1Data.finishDataWrite        ();
    
        _L1DataUpdated.clearFifoEvents();
        _bEvrDataFullUpdated = false;
        _bEvrDataIncomplete  = false;
                    
      } // else ( ! _evrL1Data.isDataWriteReady() )
    
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
      _outlet.send((char *) &datagram, _commands, _ncommands, _dst);      
    } // if (!_bReadout) 

    _bReadout  = false;
    _ncommands = 0;
  } 

  void clear()        
  { 
    //if ( evrHasEvent(_er) )
    //{ // sleep for 1 millisecond to let signal handler process FIFO events
    //  timeval timeSleepMicro = {0, 1000}; // 1 milliseconds  
    //  select( 0, NULL, NULL, NULL, &timeSleepMicro);       
    //}
    
    if (_bReadout || _ncommands) 
    {
      /*
       * This case will not happen if we have waited for the last
       * terminator
       */
      FIFOEvent fe;
      fe.TimestampHigh    = _lastFiducial;
      fe.TimestampLow     = 0;
      fe.EventCode        = TERMINATOR;      
      _bEvrDataIncomplete = true;
      startL1Accept(fe);      
    }
    _bReadout   = false;
    _ncommands  = 0;
    _L1DataUpdated.clearFifoEvents();
    _L1DataLatch  .clearFifoEvents();
    _L1DataLatchQ .clearFifoEvents();    
        
    /**
     * Note: Don't call _evrL1Data.reset() to clear the L1 Data buffer here
     *   Because the un-processed L1 Data will be sent out in the Disable action
     */
  }
  
  void reset()        
  {
    clear();    
    _evrL1Data.reset();
    _evtCounter = 0;
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
      codeState.bCommand        = eventCode.isCommand   ();
      codeState.iDefReportDelay = eventCode.reportDelay ();
      if (eventCode.isLatch())
        codeState.iDefReportWidth = -eventCode.releaseCode();             
      else
        codeState.iDefReportWidth = eventCode.reportWidth ();             
    }    
  }  
  
  int getL1Data(int iTriggerCounter, const EvrDataUtil* & pEvrData, bool& bOutOfOrder) 
  {  
    pEvrData    = NULL; // default return value: invalid data
    bOutOfOrder = false;
    
    if ( ! _evrL1Data.isDataReadReady() )
    {
      printf( "L1Xmitter::getL1Data(): No L1 Data ready. Trigger Counter: %d\n"
        "  Read Index = %d, Write Index = %d\n", 
        iTriggerCounter,
        _evrL1Data.readIndex(), _evrL1Data.writeIndex() );
      return 1; // Will set Dropped Contribution damage for L1 Data
    }    
    
    /*
     * Normal case: current data counter == trigger counter
     */
    if ( _evrL1Data.getCounterRead() == iTriggerCounter )
    {
      pEvrData = & _evrL1Data.getDataRead();    
      return 0;      
    }

    const int iFiducialWrapAroundDiffMin = 65536; 
    
    /*
     * Error cases:
     *
     * Case 1: Current data counter is later than the trigger counter
     *   
     *   In this case, we will do nothing to the existing data     
     *   We just return an error code to indicate
     *   thre is no valid L1 data returned
     */
     
    if ( _evrL1Data.getCounterRead() > iTriggerCounter && //  test if data is later than trigger
      iTriggerCounter + iFiducialWrapAroundDiffMin > _evrL1Data.getCounterRead() // special case: counter wrap-around    
    )
    {
      printf( "L1Xmitter::getL1Data(): Missing L1 Data: "
        "Trigger counter: %d, Current Data counter: %d\n"
        "  Read Index = %d, Write Index = %d\n", 
        iTriggerCounter,        _evrL1Data.getCounterRead(),
        _evrL1Data.readIndex(), _evrL1Data.writeIndex() );
      
      return 2;      
    }

    /*
     * Case 2: Current data counter is earlier than trigger counter
     *   
     *   We test if there exists a data that has the same counter as the trigger
     */
    int iDataIndex =  _evrL1Data.findDataWithCounter( iTriggerCounter );
    if ( iDataIndex != -1 )
    {
      /*
       * Case 2a: Some out-of-order data matches the trigger
       *
       *   In this case, we will return corresponding data, 
       *   and mark this data as invalid, so it will not be
       *   used later
       */      
      printf( "L1Xmitter::getL1Data(): Recovered Out-of-order L1 Data: "
        "Trigger counter: %d, Current Data counter: %d\n"
        "  Read Index = %d, Write Index = %d, Data Index = %d\n", 
        iTriggerCounter,        _evrL1Data.getCounterRead(),
        _evrL1Data.readIndex(), _evrL1Data.writeIndex(), iDataIndex );
       
      _evrL1Data.markDataAsInvalid( iDataIndex );
      pEvrData = & _evrL1Data.getDataWithIndex( iDataIndex );
      
      /* 
       *   We return a special error code to indicate that 
       *   the data is an out-of-order data, so the buffer 
       *   should not be released by client, until all earlier 
       *   data have been processed
       */
      bOutOfOrder = true;
      return 3;
    }
                 
     
    /*
     * Case 2b: No data in the buffer can match the trigger
     *
     *   In this case, we will discard the data until the remaining
     *   data is newer than the trigger
     *
     *   Then we return an error code to 
     *   indicate thre is no valid L1 data returned
     */         

    printf( "L1Xmitter::getL1Data(): Missing Triggers: "
      "Trigger counter: %d, Current Data counter: %d\n"
      "  Read Index = %d, Write Index = %d\n", 
      iTriggerCounter,        _evrL1Data.getCounterRead(),
      _evrL1Data.readIndex(), _evrL1Data.writeIndex() );
        
    int iDataDropped = 0;
    while ( 
      _evrL1Data.isDataReadReady() &&
      (
        _evrL1Data.getCounterRead() < iTriggerCounter || // test if trigger is later than data
        iTriggerCounter + iFiducialWrapAroundDiffMin <= _evrL1Data.getCounterRead() // special case: counter wrap-around
      )
    )
    {
      ++iDataDropped;
      _evrL1Data.finishDataRead();
    }
    
    printf( "L1Xmitter::getL1Data(): Dropped %d Data: "
      "Trigger counter: %d, Current Data counter: %d\n"
      "  Read Index = %d, Write Index = %d\n", 
      iDataDropped,
      iTriggerCounter,        _evrL1Data.getCounterRead(),
      _evrL1Data.readIndex(), _evrL1Data.writeIndex() );

    return 4; // Will set Dropped Contribution damage for L1 Data
  }
  
  bool                getL1DataFull      () { return _evrL1Data.getDataReadFull(); }
  bool                getL1DataIncomplete() { return _evrL1Data.getDataReadIncomplete(); }
  
  void releaseL1Data() 
  { 
    /*
     * For the first time setup:
     *
     * Set signal mask so that this thread will not be used 
     * as evr's signal handler
     */
    static bool bThreadSyncInit = false;        
    if (!bThreadSyncInit)
    {
      sigset_t sigetHoldIO;
      sigemptyset(&sigetHoldIO);
      sigaddset  (&sigetHoldIO, SIGIO);
      
      if ( pthread_sigmask(SIG_BLOCK, &sigetHoldIO, NULL) < 0 )
        perror( "L1Xmitter::releaseL1Data(): pthread_sigmask() failed" );
        
      bThreadSyncInit = true;
    }
        
    _evrL1Data.finishDataRead();     
  }
    
  //unsigned int          uFiducialPrev; // public data for checking fiducial increasing steps
  
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
  EvrDataUtil&          _L1DataUpdated;     // codes that contribute to the coming L1Accept
  EvrDataUtil&          _L1DataLatchQ;      // codes that contribute to later L1Accepts. Holding first-order transient events.
  EvrDataUtil&          _L1DataLatch;       // codes that contribute to later L1Accepts. Holding second-order transient events and first-order latch events
  EventCodeState        _lEventCodeState[guNumTypeEventCode];
  bool                  _bEvrDataFullUpdated;
  bool                  _bEvrDataIncomplete;
  unsigned              _lastFiducial;
  unsigned              _ncommands;
  char                  _commands[giMaxCommands];
  EvrL1Data             _evrL1Data;
    
  // Add Fifo event to the evrData with boundary check
  int addFifoEventCheck( EvrDataUtil& evrData, const EvrDataType::FIFOEvent& fe )
  {
    if ( (int) evrData.numFifoEvents() < giMaxNumFifoEvent )
      return evrData.addFifoEvent(fe);
    
    _bEvrDataFullUpdated = true; // set the flag and later the L1Accept will send out damage    
    return -1;
  }

  // Update Fifo event to the evrData with boundary check
  int updateFifoEventCheck( EvrDataUtil& evrData, const EvrDataType::FIFOEvent& fe )
  {
    int iNewEventIndex = evrData.updateFifoEventCheck(fe, giMaxNumFifoEvent);

    if ( (int) evrData.numFifoEvents() < giMaxNumFifoEvent ) 
      return iNewEventIndex;
    
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
  int addSpecialEvent( EvrDataUtil& evrData, const EvrDataType::FIFOEvent &feCur )
  {
    uint32_t uFiducialCur  = feCur.TimestampHigh;
    
    for (int iEventIndex = evrData.numFifoEvents()-1; iEventIndex >= 0; iEventIndex-- )
    {
      const EvrDataType::FIFOEvent& fifoEvent  = evrData.fifoEvent( iEventIndex );
      uint32_t                      uFiducial  = fifoEvent.TimestampHigh;

      if ( uFiducial == uFiducialCur ) // still in the same timestamp. this event will be checked again in later iteration.
        break;
      
      uint32_t        uEventCode = fifoEvent.EventCode;
      EventCodeState& codeState  = _lEventCodeState[uEventCode];
    
      if ( codeState.iDefReportWidth != 0 ) // interesting event
        break;
        
      evrData.removeTailEvent();      
    }
    
    addFifoEventCheck( evrData, feCur );       
    return 0;
  }
  
  void addCommand( const FIFOEvent& fe )
  {
    if (_ncommands < giMaxCommands) 
    {
      _commands[_ncommands++] = fe.EventCode;
    }
    else 
    {
      printf("Dropped command %d\n",fe.EventCode);
    }
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
  EvrL1Action(Evr & er, const Src& src) :
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

      int                iTriggerCounter  = in->seq.stamp().vector();      
      bool               bDataFull        = l1xmitGlobal->getL1DataFull();
      bool               bDataInc         = l1xmitGlobal->getL1DataIncomplete();
      
      const EvrDataUtil* pEvrData         = NULL;
      bool               bOutOfOrder      = false;
      
      l1xmitGlobal->getL1Data(iTriggerCounter, pEvrData, bOutOfOrder);
      
      bool bNoL1Data = ( pEvrData == NULL );
      if ( bNoL1Data )
      {
        static const EvrDataUtil evrDataEmpty( 0, NULL );      
        pEvrData = &evrDataEmpty;        
      }
      
      const EvrDataUtil& evrData = *pEvrData;
      
      out = 
       new ( &_poolEvrData ) CDatagram( in->datagram() ); 
      out->datagram().xtc.alloc( sizeof(Xtc) + evrData.size() );
      
      //unsigned int  uFiducialPrev = l1xmitGlobal->uFiducialPrev;
      //unsigned int  uFiducialCur  = out->datagram().seq.stamp().fiducials();  
      
      //if ( uFiducialPrev != 0 )
      //{
      //  const int iFiducialWrapAroundDiffMin = 65536; 
      //  if ( (uFiducialCur <= uFiducialPrev && uFiducialPrev < uFiducialCur+iFiducialWrapAroundDiffMin) || 
      //    uFiducialCur > uFiducialPrev + 360 )
      //  {
      //    //printf( "EvrL1Action::fire(): seq 0x%x followed 0x%x\n", uFiducialCur, uFiducialPrev );
      //    out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      //  }
      //}
      //l1xmitGlobal->uFiducialPrev = uFiducialCur;
      
      if ( bDataFull )
      {
        printf( "EvrL1Action::fire(): Too many FIFO events recieved, so L1 data buffer is full and incomplete.\n" );
        printf( "  Please check the readout and terminator event settings.\n" );        
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      }

      if ( bDataInc )
      {
        printf( "EvrL1Action::fire(): Data incomplete, because disable action occurs before the terminator event comes\n" );        
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
        
        //if ( evrHasEvent(_er) )
        //{
          //printf( "EvrL1Action::fire(): Found unprocessed FIFO event\n" );
          // mark the damage user bit 0 to indicate there are unprocessed FIFO events
          //out->datagram().xtc.damage.userBits(0x1);
        //}
      }

      if ( bNoL1Data )
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
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
      
      if ( !bNoL1Data )
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
    // reset the fiducial checking counter
    //l1xmitGlobal->uFiducialPrev = 0; 
    
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

    l1xmitGlobal->clear();
    
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
  EvrConfigManager(Evr & er, CfgClientNfs & cfg, Appliance& app, bool bTurnOffBeamCodes):
    _er (er),
    _cfg(cfg),
    _cfgtc(_evrConfigType, cfg.src()),
    _configBuffer(
      new char[ giMaxCalibCycles* evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] ),
    _cur_config(0),
    _end_config(0),
    _occPool   (new GenericPool(sizeof(UserMessage),1)),
    _app       (app),
    _bTurnOffBeamCodes(_bTurnOffBeamCodes)
  {
  }

   ~EvrConfigManager()
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

    const EvrConfigType& cfg = *_cur_config;

    printf("Configuring evr : %d/%d/%d\n",
           cfg.npulses(),cfg.neventcodes(),cfg.noutputs());

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

      printf("pulse %d :%d %c %d/%d/%d\n",
       k, pc.pulseId(), pc.polarity() ? '-':'+', 
       pc.prescale(), pc.delay(), pc.width());
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

      printf("output %d : %d %x\n", k, map.conn_id(), map.map());
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

      printf("event %d : %d %x/%x/%x\n",
       uEventIndex, eventCode.code(), 
       eventCode.maskTrigger(),
       eventCode.maskSet(),
       eventCode.maskClear());
    }
    
    if (!_bTurnOffBeamCodes)
    {
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BEAM,  enable);
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BYKIK, enable);
    }
    
    _er.SetFIFOEvent(ram, TERMINATOR, enable);

    unsigned dummyram = 1;
    _er.MapRamEnable(dummyram, 1);
    
    l1xmitGlobal->setEvrConfig( &cfg );
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

    int len = _cfg.fetch(*tr, _evrConfigType, _configBuffer);
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

    _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
    _end_config = _configBuffer+len;
    _cfgtc.extent = sizeof(Xtc) + _cur_config->size();

    delete msg;
    _cfgtc.damage = 0;
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
  EvrCfgClient    _cfg;
  Xtc             _cfgtc;
  char *          _configBuffer;
  const EvrConfigType* _cur_config;
  const char*          _end_config;
  GenericPool*         _occPool;
  Appliance&           _app;
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
    FIFOEvent fe;
    while( ! er.GetFIFOEvent(&fe) )
      l1xmitGlobal->xmit(fe);
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
  EvrConfigManager* cmgr = new EvrConfigManager(_er, cfg, _fsm, bTurnOffBeamCodes);

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

// check if evr has any unprocessed fifo event
//bool evrHasEvent(Evr& er)
//{    
//  uint32_t& uIrqFlagOrg = *(uint32_t*) ((char*) &er + 8);  
//  uint32_t  uIrqFlagNew = be32_to_cpu(uIrqFlagOrg);
//  
//  if ( uIrqFlagNew & EVR_IRQFLAG_EVENT)
//    return true;
//  else
//    return false;
//}

const int EvrManager::EVENT_CODE_BEAM;  // value is defined in the header file
const int EvrManager::EVENT_CODE_BYKIK; // value is defined in the header file
