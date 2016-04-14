#include "pds/evgr/EvrSync.hh"
#include "pds/evgr/EvrSyncRoutine.hh"
#include "pds/evgr/EvrFIFOHandler.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Client.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"

#define DBG
//#define ONE_PHASE

//  The latest eventcodes are 40-46
//     40 = 12954
//     41 = 12964
//        ..
//     46 = 13014
//
//  This provides the greatest window before
//  the next used eventcodes
//
static const unsigned SyncCode        = Pds::EvrSyncMaster::EVENT_CODE_SYNC;
static const unsigned TermCode        =  1;
static const unsigned EnableDelay     =  7;
static const unsigned DisableDelay    =  7;
static const int      ram             =  0;
static const int      dummyram        =  1;


namespace Pds {
  class ReleaseRoutine : public Routine {
  public:
    ReleaseRoutine(EvrFIFOHandler& fh) : _fh(fh) {}
    virtual ~ReleaseRoutine() {}
  public:
    void routine() {
      _fh.release_sync();
      delete this;
#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("release_sync complete at %d.%09d\n",int(ts.tv_sec),int(ts.tv_nsec));
#endif
    }
  private:
    EvrFIFOHandler& _fh;
  };
};


using namespace Pds;

EvrSyncMaster::EvrSyncMaster(EvrFIFOHandler& fifo_handler,
                             Evr&            er,
                             unsigned        partition,
                             Task*           task,
                             Client&         outlet,
                             Appliance&      app) :
  _fifo_handler(fifo_handler),
  _er          (er),
  _state       (Disabled),
  _dst         (StreamPorts::evr(partition)),
  _task        (*task),
  _outlet      (outlet),
  _app         (app),
  _pool        (sizeof(Occurrence),1),
  _target      (0)
{
  //  Empty FIFO (out of context)
  FIFOEvent tfe;
  while( !_er.GetFIFOEvent(&tfe) )
    ;

}

EvrSyncMaster::~EvrSyncMaster()
{
}

void EvrSyncMaster::initialize(bool enable)
{
  if (enable) {
    _state = EnableInit;

    // configure EVR to listen for synchronization eventcode
    int dummyram = 1;
    _er.SetFIFOEvent(dummyram, SyncCode, 1);
    _er.SetFIFOEvent(dummyram, TermCode, 1);
  }
  else {
    _state = DisableInit;
  }
}

bool EvrSyncMaster::handle(const FIFOEvent& fe)
{
  if (_state == EnableInit) {
    if (fe.EventCode == SyncCode) {

      //  Empty FIFO
      FIFOEvent tfe;
      while( !_er.GetFIFOEvent(&tfe) )
        ;

      _target = fe.TimestampHigh;
      if (_target == 0)
        return true;

      _target += EnableDelay;
      if (_target > TimeStamp::MaxFiducials)
        _target -= TimeStamp::MaxFiducials;

      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ClockTime ctime(ts.tv_sec, ts.tv_nsec);
      TimeStamp stamp(fe.TimestampLow, _target, SyncCode);
      Sequence seq(Sequence::Occurrence, TransitionId::Enable, ctime, stamp);
      EvrDatagram datagram(seq, -1, 0);
      _outlet.send((char *) &datagram, 0, 0, _dst);
#ifdef DBG
      printf("EvrMasterFIFOHandler enable sync target %x at %d.%09d\n",
       _target, int(ts.tv_sec), int(ts.tv_nsec));
#endif
      _state = EnableSeek;
    }
    return true;
  }

  if (_state == EnableSeek) {
    if (fe.EventCode == TermCode) {
      unsigned dtarget = (fe.TimestampHigh
        + TimeStamp::MaxFiducials
        - _target)%TimeStamp::MaxFiducials;
      if (dtarget > EnableDelay &&
    dtarget < TimeStamp::MaxFiducials-EnableDelay)
  {
    printf("Enable sync quit with fiducial %x (target %x)\n",
     fe.TimestampHigh, _target);

  }
      else if (dtarget == 0)
  ;
      else
  return true;

      _state = Enabled;
//       _er.SetFIFOEvent(dummyram, SyncCode, 0);
//       _er.SetFIFOEvent(dummyram, TermCode, 0);

      //  Empty FIFO
      FIFOEvent tfe;
      while( !_er.GetFIFOEvent(&tfe) )
  ;

      //  Enable Triggers
      _er.MapRamEnable(ram, 1);

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrMasterFIFOHandler enabled at %d.%09d\n",
       int(ts.tv_sec), int(ts.tv_nsec));
#endif

      //  Generate EVR Enabled Occurrence
      _app.post(new (&_pool) Occurrence(OccurrenceId::EvrEnabled));
    }
    return true;
  }

  if (_state == DisableInit) {
    if (fe.EventCode == SyncCode) {
      _target = fe.TimestampHigh + DisableDelay;
      if (_target > TimeStamp::MaxFiducials)
  _target -= TimeStamp::MaxFiducials;

      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ClockTime ctime(ts.tv_sec, ts.tv_nsec);
      TimeStamp stamp(fe.TimestampLow, _target, SyncCode);
      Sequence seq(Sequence::Occurrence, TransitionId::Disable, ctime, stamp);
      EvrDatagram datagram(seq, -1, 0);
      _outlet.send((char *) &datagram, 0, 0, _dst);

#ifdef DBG
      printf("EvrMasterFIFOHandler disable sync target %x at %d.%09d\n",
       _target, int(ts.tv_sec), int(ts.tv_nsec));
#endif
      _state = DisableSeek;
    }
  }

  else if (_state == DisableSeek) {
    if (fe.EventCode == TermCode && fe.TimestampHigh==_target) {

      _state = Disabled;

      //  Disable Triggers
      _er.SetFIFOEvent(dummyram, SyncCode, 0);
      _er.SetFIFOEvent(dummyram, TermCode, 0);
      _er.MapRamEnable(dummyram, 1);

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrMasterFIFOHandler disabled at %d.%09d\n",
       int(ts.tv_sec), int(ts.tv_nsec));
#endif

      _task.call(new ReleaseRoutine(_fifo_handler));

      //  Empty FIFO
      //      FIFOEvent tfe;
      //      while( !_er.GetFIFOEvent(&tfe) )
      //  ;
    }
  }
  return false;
}


EvrSyncSlave::EvrSyncSlave(EvrFIFOHandler& fifo_handler,
                           Evr&            er,
                           unsigned        partition,
                           Task*           task,
                           Task*           sync_task) :
  _fifo_handler(fifo_handler),
  _er          (er),
  _state       (Disabled),
  _target      (0),
  _task        (*task),
  _sync_task   (*sync_task),
  _routine     (new EvrSyncRoutine(partition,*this))
{
  _sync_task.call(_routine);
}

EvrSyncSlave::~EvrSyncSlave()
{
  delete _routine;
}

void EvrSyncSlave::initialize(unsigned target,
                              bool     enable)
{
  if (enable) {
    _state  = EnableInit;
    _target = target;

    // configure EVR to listen for synchronization eventcode
    int dummyram = 1;
    _er.SetFIFOEvent(dummyram, TermCode, 1);
  }
  else {
    _state  = DisableInit;
    _target = target;
  }
}

void EvrSyncSlave::enable()
{
}

bool EvrSyncSlave::handle(const FIFOEvent& fe)
{
  if (_state == Enabled)
    return false;

  if (_state == EnableInit) {
    if (fe.EventCode == TermCode) {

      if (fe.TimestampHigh!=0)
  _state = EnableSeek;

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler enable sync code at %x : %d.%09d\n",
       fe.TimestampHigh, int(ts.tv_sec), int(ts.tv_nsec));
#endif
    }
    return true;
  }

  if (_state == EnableSeek) {
    if (fe.EventCode == TermCode && fe.TimestampHigh==_target) {

      _state = Enabled;

      _er.SetFIFOEvent(dummyram, TermCode, 0);

      //  Empty FIFO
      FIFOEvent tfe;
      while( !_er.GetFIFOEvent(&tfe) )
  ;

      //  Enable Triggers
      _er.MapRamEnable(ram, 1);

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler enabled at %d.%09d\n",
       int(ts.tv_sec), int(ts.tv_nsec));
#endif
    }
    return true;
  }

  if (_state == DisableInit) {
    if (fe.EventCode == TermCode) {

      if (fe.TimestampHigh!=0)
  _state = DisableSeek;

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler disable sync code at %x : %d.%09d\n",
       fe.TimestampHigh, int(ts.tv_sec), int(ts.tv_nsec));
#endif
    }
  }

  if (_state == DisableSeek) {
    if (fe.EventCode == TermCode && fe.TimestampHigh==_target) {

      _state = Disabled;

      //  Disable Triggers
      _er.MapRamEnable(dummyram, 1);

#ifdef DBG
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler disabled at %d.%09d\n",
       int(ts.tv_sec), int(ts.tv_nsec));
#endif

      _task.call(new ReleaseRoutine(_fifo_handler));
    }
  }

  return false;
}
