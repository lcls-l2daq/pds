#include "pds/usdusb/Manager.hh"
#include "pds/usdusb/Server.hh"

#include "pds/config/UsdUsbDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "usdusb4/include/libusdusb4.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

namespace Pds {
  namespace UsdUsb {
    class Reader : public Routine {
    public:
      Reader(unsigned dev, Server& server, Task* task) :
	_dev    (dev),
	_task   (task),
	_server (server),
	_count  (0),
	_disable(true) {
	_sleepTime.tv_sec = 0;
	_sleepTime.tv_nsec = (long) 0.5e9;  //0.5 sec
      }

      void resetCount() {_server.resetCount();}
  
      Task& task() { return *_task; }
      void enable () { _disable=false; _task->call(this);}
      void disable() { _disable=true ; }
  
      virtual ~Reader() {}
      void routine() {
	if (_disable) {
	  ;
	}
	else {
	  long nRecords = MAX_RECORDS;
	  const int tmo = 100; // milliseconds
	  int status = USB4_ReadFIFOBufferStruct(_dev, 
						 &nRecords,
						 _records,
						 tmo);
	  if (status != USB4_SUCCESS) {
	    printf("ReadFIFO result %d\n",status);
	  }
	  else {
	    _server.post((char*)_records, nRecords*sizeof(UsdUsbDataType));
	  }
	  _task->call(this);
	}
      } 
 
    private:
      unsigned    _dev;
      Task*       _task;
      Server&     _server;
      unsigned    _count;
      bool        _disable; 
      timespec    _sleepTime;
      enum { MAX_RECORDS = 1024 };
      USB4_FIFOBufferRecord _records[MAX_RECORDS];
    };

    class Enable : public Routine {
    public:
      Enable(Reader& reader): _reader(reader), _sem(Semaphore::EMPTY) { }
      ~Enable() { }
      void call   () { _reader.task().call(this); _sem.take(); }
      void routine() { _reader.enable(); _sem.give(); }
    private:
      Reader&    _reader;
      Semaphore  _sem;
    };

    class Disable : public Routine {
    public:
      Disable(Reader& reader): _reader(reader), _sem(Semaphore::EMPTY) { }
      ~Disable() { }  
      void call   () { _reader.task().call(this);  _sem.take(); }
      void routine() { _reader.disable(); _sem.give(); }
    private:
      Reader&    _reader;
      Semaphore  _sem;
    };

    class AllocAction : public Action {
    public:
      AllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
      Transition* fire(Transition* tr) {
	const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
	_cfg.initialize(alloc.allocation());
	return tr;
      }
    private:
      CfgClientNfs& _cfg;
    };


    class L1Action : public Action,
		     public Pds::XtcIterator {
    public:
      L1Action() : _lreset(true) {}
      InDatagram* fire(InDatagram* in) {
	unsigned dgm_ts = in->datagram().seq.stamp().fiducials();
	_nfid   = dgm_ts - _dgm_ts;
	if (_nfid < 0)
	  _nfid += Pds::TimeStamp::MaxFiducials;

	_in = in;
	iterate(&in->datagram().xtc);

	return in;
      }
      void reset() { _lreset=true; }
      int process(Xtc* xtc) {
	if (xtc->contains.id()==TypeId::Id_Xtc)
	  iterate(xtc);
	else if (xtc->contains.value() == _usdusbDataType.value()) {
	  const UsdUsbDataType& data = *reinterpret_cast<const UsdUsbDataType*>(xtc->payload());
	  unsigned usd_ts = data.timestamp();
	  if (_lreset) {
	    _lreset = false;
	    _dgm_ts = _in->datagram().seq.stamp().fiducials();
	    _usd_ts = usd_ts;
	  }
	  else {
	    const double clkratio  = 357./48e6;
	    const double tolerance = 0.03;  // AC line rate jitter
	    const unsigned maxdfid = 20000; // 48MHz timestamp rollover

	    double fdelta = double(usd_ts - _usd_ts)*clkratio/double(_nfid) - 1;
	    if (fabs(fdelta) > tolerance && _nfid < maxdfid) {
	      unsigned nfid = unsigned(double(usd_ts - _usd_ts)*clkratio + 0.5);
	      printf("  timestep error: fdelta %f  dfid %d  tds %u,%u [%u]\n",
		     fdelta, _nfid, usd_ts, _usd_ts, nfid);
	      _in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
	    }
	    else {
	      _dgm_ts = _in->datagram().seq.stamp().fiducials();
	      _usd_ts = usd_ts;
	    }
	  }
	  return 0;
	}
	return 1;
      }
    private:
      bool     _lreset;
      unsigned _usd_ts;
      unsigned _dgm_ts;
      unsigned _nfid;
      InDatagram* _in;
    };


    class ConfigAction : public Action {
      enum {MaxConfigSize=0x100000};
    public:
      ConfigAction(unsigned dev, Reader& reader,
		   L1Action& l1,
		   CfgClientNfs& cfg,
		   Manager& mgr) :
	_dev   (dev),
	_reader(reader),
	_l1    (l1),
	_enable(reader),
	_cfgtc(_usdusbConfigType,cfg.src()),
	_cfg(cfg),
	_mgr(mgr), 
	_occPool(sizeof(UserMessage),1) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) {
	// insert assumes we have enough space in the input datagram
	dg->insert(_cfgtc, &_config);
	if (_nerror) {
	  printf("*** Found %d configuration errors\n",_nerror);
	  dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
	}
	return dg;
      }
      Transition* fire(Transition* tr) {

	_nerror = 0;
	int len = _cfg.fetch( *tr,
			      _usdusbConfigType,
			      &_config,
			      sizeof(_config) );
	if (len <= 0) {
         printf("ConfigAction: failed to retrieve configuration "
                ": (%d) %s.  Applying default.\n",
                errno,
                strerror(errno) );

	 UserMessage* msg = new (&_occPool) UserMessage;
	 msg->append("USB4: failed to retrieve configuration.\n");
	 _mgr.appliance().post(msg);

         _nerror += 1;
	}
	else {
	  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
	    _nerror += USB4_SetMultiplier    (_dev, i, (int)_config.quadrature_mode(i)) != USB4_SUCCESS;
	    _nerror += USB4_SetCounterMode   (_dev, i, (int)_config.counting_mode  (i)) != USB4_SUCCESS;
	    _nerror += USB4_SetForward       (_dev, i, 0) != USB4_SUCCESS; // A/B assignment (normal)
	    _nerror += USB4_SetCaptureEnabled(_dev, i, 1) != USB4_SUCCESS;
	    _nerror += USB4_SetCounterEnabled(_dev, i, 1) != USB4_SUCCESS;
	  }

	  // Clear the FIFO buffer
	  USB4_ClearFIFOBuffer(_dev);
	
	  // Enable the FIFO buffer
	  USB4_EnableFIFOBuffer(_dev);
	
	  // Clear the captured status register
	  USB4_ClearCapturedStatus(_dev, 0);

	  static int DIN_CFG[] = { 0, 0, 0, 0, 0, 0, 0, 1 };
	  USB4_SetDigitalInputTriggerConfig(_dev, DIN_CFG, DIN_CFG);

	  USB4_ClearDigitalInputTriggerStatus(_dev);

	  _nerror = 0;  // override

	  if (_nerror) {

	    UserMessage* msg = new (&_occPool) UserMessage;
	    msg->append("USB4: failed to apply configuration.\n");
	    _mgr.appliance().post(msg);

	  }
	  else {
	    _l1    .reset();
	    _reader.resetCount();
	    _enable.call();
	  }
	}
	return tr;
      }
    private:
      unsigned         _dev;
      Reader&          _reader;  
      L1Action&        _l1;
      Enable           _enable;
      UsdUsbConfigType _config;
      Xtc              _cfgtc;
      CfgClientNfs&    _cfg;
      Manager&         _mgr;
      GenericPool      _occPool;
      unsigned         _nerror;
    };

    class UnconfigureAction : public Action {
    public:
      UnconfigureAction(Reader& reader) : 
	_disable(reader) { }
      InDatagram* fire(InDatagram* in) {
	_disable.call();
	return in;
      }
    private:
      Disable _disable;
    };

  }
}

using namespace Pds::UsdUsb;

Manager::Manager(unsigned dev, Server& server, CfgClientNfs& cfg) :
  _fsm(*new Fsm) 
{

  Task* task = new Task(TaskObject("UsdReadout",35));
  Reader& reader = *new Reader(dev, server,task);
  L1Action* l1 = new L1Action;
  
  _fsm.callback(TransitionId::Map, new AllocAction(cfg)); 
  _fsm.callback(TransitionId::Configure,new ConfigAction(dev, reader, *l1, cfg, *this));
  _fsm.callback(TransitionId::L1Accept,l1);
  _fsm.callback(TransitionId::Unconfigure,new UnconfigureAction(reader));
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}
