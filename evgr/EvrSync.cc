#include "EvrSync.hh"
#include "EvrFIFOHandler.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Client.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/NetServer.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/Mtu.hh"
#include "pds/collection/Route.hh"

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
  class SyncRoutine : public Routine {
  public:
    SyncRoutine(Evr&     er, 
		unsigned partition,
		EvrSyncSlave& sync) :
      _er(er), 
      _group (StreamPorts::evr(partition)),
      _server((unsigned)-1,
	      _group,
	      sizeof(EvrDatagram),
	      Mtu::Size,
	      1),
      _sync  (sync)
    {
      _server.join(_group,Ins(Route::interface()));
    }
    virtual ~SyncRoutine()
    {
      close(_server.socket());
    }
  public:
    void routine()
    {
      const int bsiz = 256;
      char* payload = new char[bsiz];

      int len = _server.fetch( payload, MSG_WAITALL );
      if (len ==0) {
	const EvrDatagram* dg = reinterpret_cast<const EvrDatagram*>(_server.datagram());
	
	_sync.initialize(dg->seq.stamp().fiducials(),
			 dg->seq.service()==TransitionId::Enable);
	
	/*
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("sync mcast seq %d.%09d (%x : %d.%09d)\n",
	       int(ts.tv_sec), int(ts.tv_nsec),
	       dg->seq.stamp().fiducials(),
	       dg->seq.clock().seconds(),
	       dg->seq.clock().nanoseconds());
	*/
      }
    }
  private:
    Evr& _er;
    Ins  _group;
    NetServer _server;
    EvrSyncSlave& _sync;
  };

  class ReleaseRoutine : public Routine {
  public:
    ReleaseRoutine(EvrFIFOHandler& fh) : _fh(fh) {}
    virtual ~ReleaseRoutine() {}
  public:
    void routine() {
      _fh.release_sync();
      delete this;
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
			     Client&         outlet) :
  _fifo_handler(fifo_handler),
  _er          (er),
  _state       (Disabled),
  _dst         (StreamPorts::evr(partition)),
  _task        (*task),
  _outlet      (outlet),
  _target      (0)
{
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
      /*
      printf("EvrMasterFIFOHandler enable sync target %x at %d.%09d\n",
	     _target, int(ts.tv_sec), int(ts.tv_nsec));
      */
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
      _er.SetFIFOEvent(dummyram, SyncCode, 0);
      _er.SetFIFOEvent(dummyram, TermCode, 0);
      
      //  Empty FIFO
      FIFOEvent tfe;
      while( !_er.GetFIFOEvent(&tfe) )
	;
	
      //  Enable Triggers
      _er.MapRamEnable(ram, 1);

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrMasterFIFOHandler enabled at %d.%09d\n",
	     int(ts.tv_sec), int(ts.tv_nsec));
      */
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

      /*
      printf("EvrMasterFIFOHandler disable sync target %x at %d.%09d\n",
	     _target, int(ts.tv_sec), int(ts.tv_nsec));
      */
      _state = DisableSeek;
    }
  }

  else if (_state == DisableSeek) {
    if (fe.EventCode == TermCode && fe.TimestampHigh==_target) {

      _state = Disabled;

      //  Disable Triggers
      _er.MapRamEnable(dummyram, 1);

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrMasterFIFOHandler disabled at %d.%09d\n",
	     int(ts.tv_sec), int(ts.tv_nsec));
      */

      _task.call(new ReleaseRoutine(_fifo_handler));
    }
  }
  return false;
}


EvrSyncSlave::EvrSyncSlave(EvrFIFOHandler& fifo_handler,
			   Evr&            er, 
			   unsigned        partition, 
			   Task*           task) :
  _fifo_handler(fifo_handler),
  _er          (er),
  _state       (Disabled),
  _target      (0),
  _task        (*task),
  _routine     (new SyncRoutine(er,partition,*this))
{
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

    _task.call(_routine);
  }
  else {
    _state  = DisableInit;
    _target = target;
  }
}

void EvrSyncSlave::enable()
{
  _task.call(_routine);
}

bool EvrSyncSlave::handle(const FIFOEvent& fe)
{
  if (_state == Enabled)
    return false;

  if (_state == EnableInit) {
    if (fe.EventCode == TermCode) {

      if (fe.TimestampHigh!=0)
	_state = EnableSeek;

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler enable sync code at %x : %d.%09d\n",
	     fe.TimestampHigh, int(ts.tv_sec), int(ts.tv_nsec));
      */
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

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler enabled at %d.%09d\n",
	     int(ts.tv_sec), int(ts.tv_nsec));
      */
    }
    return true;
  }

  if (_state == DisableInit) {
    if (fe.EventCode == TermCode) {

      if (fe.TimestampHigh!=0)
	_state = DisableSeek;

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler disable sync code at %x : %d.%09d\n",
	     fe.TimestampHigh, int(ts.tv_sec), int(ts.tv_nsec));
      */
    }
  }

  if (_state == DisableSeek) {
    if (fe.EventCode == TermCode && fe.TimestampHigh==_target) {

      _state = Disabled;

      //  Disable Triggers
      _er.MapRamEnable(dummyram, 1);

      /*
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      printf("EvrSlaveFIFOHandler disabled at %d.%09d\n",
	     int(ts.tv_sec), int(ts.tv_nsec));
      */

      _task.call(new ReleaseRoutine(_fifo_handler));
    }
  }

  return false;
}
