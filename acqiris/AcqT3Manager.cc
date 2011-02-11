// This is how the acqiris readout software is structured.  It is
// not completely straight-forward, because it attempts to make the
// acqiris driver play well with our event-builder, with no extra
// copying of the data, and in a manner that does not unnecessarily
// block the event-builder thread.

// In the Configure Action, an AcqReader thread is started, where
// it is waiting for an acquisition to be completed, meaning that
// acqiris data is waiting to be DMAd.  The acquisition is triggered by
// an EVR signal (which is enabled on the Enable transition).  Both
// the wait-for-acquisition-completion (AcqReader) and wait-for-dma-completion
// (AcqDma) run in the same thread.
 
// Now we discuss how the actual trigger/readout mechanism works...
// After the acquisition is completed,  the AcqReader calls
// AcqServer::headerComplete.  This writes to a pipe that unblocks
// AcqServer::fetch (called by the event-builder).  This copies
// the header information into the correct XTC area, and starts the
// AcqDma, pointing AcqDma to the correct XTC area for the bulk acqiris
// data.  The call to read the data in AcqDma blocks until the DMA has
// completed.  At this point AcqDma calls AcqReader::payloadComplete().
// This in turn writes to the pipe, which unblocks the event-builder
// fetch and completes the event.  AcqDma then puts AcqReader back in
// the queue of the thread in order to get ready for the next event.
// - cpo, Feb. 4, 2009.
 
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "AcqT3Manager.hh"
#include "AcqManager.hh"
#include "AcqServer.hh"
#include "acqiris/aqdrv4/AcqirisImport.h"
#include "acqiris/aqdrv4/AcqirisT3Import.h"
#include "pds/service/GenericPool.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/SysClk.hh"
#include "pds/utility/DmaEngine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/AcqDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pdsdata/xtc/DetInfo.hh"

#define ACQ_TIMEOUT_MILISEC 4000
//#define DBUG

using namespace Pds;

static unsigned nprint =0;

static void dumpStatus(ViSession instrumentId, ViStatus status, const char* ctxt)
{
  const int messageSize=256;
  char message[messageSize];
  if (status != VI_SUCCESS) {
    Acqrs_errorMessage(instrumentId,status,message,messageSize);
    printf("%s : %s\n",ctxt,message);
  }
}

class AcqT3Reader;

class AcqT3Enable : public Routine {
public:
  AcqT3Enable(AcqT3Reader& reader): _acqReader(reader), _sem(Pds::Semaphore::EMPTY) { }
  ~AcqT3Enable() { }
  void routine();
  void wait() { _sem.take(); }
private:
  AcqT3Reader& _acqReader;
  Pds::Semaphore _sem;
};

class AcqT3Disable : public Routine {
public:
  AcqT3Disable(AcqT3Reader& reader): _acqReader(reader), _sem(Pds::Semaphore::EMPTY) { }
  ~AcqT3Disable() { }  
  void routine();
  void wait() { _sem.take(); }
private:
  AcqT3Reader& _acqReader;
  Pds::Semaphore _sem;
};

class AcqT3Reader : public Routine {
  enum { ArraySize=8*1024*1024 };
public:
  AcqT3Reader(ViSession instrumentId, 
	      AcqT3Manager& mgr,
	      AcqServer& server, 
	      Task* task) :
    _instrumentId(instrumentId),
    _task        (task),
    _mgr         (mgr),
    _occPool     (sizeof(Occurrence),1),
    _server      (server),
    _event       (0),
    _count       (0),
    _buffer      (new char[ArraySize]),
    _acqReadEnb  (false) 
  {
    _sleepTime.tv_sec = 0;
    _sleepTime.tv_nsec = (long) 0.5e9;  //0.5 sec
  }
  ~AcqT3Reader() { delete[] _buffer; }
public:
  void resetCount() {_event=0; _outoforder=false;}
  void start() {_task->call(this);}
  
  void acqControl(AcqT3Enable*  acqEnable, 
		  AcqT3Disable* acqDisable) {
    _acqEnable  = acqEnable;
    _acqDisable = acqDisable;
  }
  void enableAcqRead()  {_task->call((Routine*)_acqEnable ); _acqEnable ->wait(); }
  void disableAcqRead() {_task->call((Routine*)_acqDisable); _acqDisable->wait(); }  
  void enable()  {_acqReadEnb = true;} 
  void disable() {
    ViStatus status = AcqrsT3_stopAcquisition(_instrumentId);
    dumpStatus(_instrumentId, status, "AcqT3Reader::stop");
    _acqReadEnb = false;
    _count = 0;
  }
  
  void routine() {
    ViStatus status;
	
    if (_acqReadEnb) {
      AcqrsT3_acquire(_instrumentId);		
      status = AcqrsT3_waitForEndOfAcquisition(_instrumentId,ACQ_TIMEOUT_MILISEC);
      if (status == (int)ACQIRIS_ERROR_ACQ_TIMEOUT) {
#ifdef DBUG
	dumpStatus(_instrumentId, status,"AcqT3Reader::wait");
#endif
        _task->call(this);
      }
      else {
	AqT3ReadParameters readParam;
	::memset(&readParam, 0, sizeof(readParam));
	readParam.dataArray = _buffer;
	readParam.dataSizeInBytes = ArraySize;
	readParam.nbrSamples = 0;
	readParam.dataType = ReadInt32;
	readParam.readMode = AqT3ReadContinuous;

        AqT3DataDescriptor dataDesc;
        ::memset(&dataDesc, 0, sizeof(dataDesc));

	//
	//  I worry that in the time between calls to AcqrsT3_acquire
	//  we are 'dead'.  Maybe move this to another thread.
	//
	status = AcqrsT3_readData(_instrumentId, 0, &readParam, &dataDesc);
	dumpStatus(_instrumentId, status,"AcqT3Reader::readData");
	if (!_outoforder) {
#ifdef DBUG
	  printf("event 0x%x\n",_event);
#endif
	  long countLast = _count;
	  for(int i=0; i<dataDesc.nbrSamples; i++) {
	    long sample = ((long *)dataDesc.dataPtr)[i];
#ifdef DBUG
	    printf("data %d : %08x\n", i, unsigned(sample));
#endif
	    long flag = (sample & 0x80000000) >> 31;
	    long channel = (sample & 0x70000000) >> 28;
	    if (flag == 0 && channel == 0) {
	      long count = sample & 0x0FFFFFFF;
	      if (count != ++countLast) {
		printf("AcqT3Reader: Error: Gap in common count between %ld and %ld\n", countLast, count);
		_outoforder=true;
	      }
	    }
	  }
	  if (_outoforder) {
	    Pds::Occurrence* occ = new (&_occPool)
	      Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
	    _mgr.appliance().post(occ);
	  }
	  else
	    _count = countLast;
	}
	_payloadSize = dataDesc.nbrSamples*4;
	_server.headerComplete(_payloadSize,_event++,
			       Damage(_outoforder ? (1<<Damage::OutOfOrder):0));
      }
    } else {
      nanosleep(&_sleepTime,NULL);
      _task->call(this);
    }	
  } 
  void copy(void* p) { memcpy(p,_buffer,_payloadSize); }
private:
  ViSession     _instrumentId; 
  Task*         _task;
  AcqT3Manager& _mgr;
  GenericPool   _occPool;
  AcqServer&    _server;
  unsigned      _event;
  long          _count;
  unsigned      _payloadSize;
  char*         _buffer;
  unsigned      _outoforder;
  bool          _acqReadEnb; 
  AcqT3Enable*  _acqEnable;
  AcqT3Disable* _acqDisable;
  timespec      _sleepTime;
};


void AcqT3Enable::routine()  { _acqReader.enable (); _sem.give(); }

void AcqT3Disable::routine() { _acqReader.disable(); _sem.give(); }

class AcqT3Dma : public DmaEngine {
public:
  AcqT3Dma(AcqT3Reader& reader, AcqServer& server,Task* task) :
    DmaEngine(task),_reader(reader),_server(server) {}
  void routine() {
    _reader.copy(_destination);
    _server.payloadComplete();  	
    _task->call(&_reader);
  }
private:
  AcqT3Reader& _reader;
  AcqServer& _server;
};

class AcqT3AllocAction : public Action {
public:
  AcqT3AllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
  Transition* fire(Transition* tr) {
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    _cfg.initialize(alloc.allocation());
    return tr;
  }
private:
  CfgClientNfs& _cfg;
};

class AcqT3ConfigAction : public Action {
  enum {MaxConfigSize=0x100000};
public:
  AcqT3ConfigAction(ViSession instrumentId, 
		    AcqT3Reader& reader,
		    const Src& src, 
		    CfgClientNfs& cfg,
		    AcqT3Manager& mgr) :
    _instrumentId(instrumentId),
    _reader      (reader),
    _cfgtc       (_acqTdcConfigType,src),
    _cfg         (cfg), _mgr(mgr), 
    _occPool     (sizeof(UserMessage),1),
    _firstTime   (1) {}
  ~AcqT3ConfigAction() {}
public:
  InDatagram* fire(InDatagram* dg) {
    // insert assumes we have enough space in the input datagram
    dg->insert(_cfgtc, &_config);
    if (_nerror) {
      printf("*** Found %d acqiris configuration errors\n",_nerror);
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }
  Transition* fire(Transition* tr) {

    // get configuration from database
    UserMessage* msg = new(&_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
    msg->append(":");

    _nerror=0;
    int len = _cfg.fetch(*tr,_acqTdcConfigType, &_config, sizeof(_config));
    if (len <= 0) {
      printf("AcqT3ConfigAction: failed to retrieve configuration : (%d)%s.  Applying default.\n",errno,strerror(errno));
      msg->append("Failed to open config file\n");
      _nerror++;
    }
    //    _config.dump();

    ViInt32 modeContinuous = 2;
    ViInt32 multipleAcqs   = 1;
    ViInt32 singleAcq      = 0;
    ViInt32 internalRefClk = 0;
    ViStatus status = AcqrsT3_configMode(_instrumentId, modeContinuous, multipleAcqs, internalRefClk);
    //    ViStatus status = AcqrsT3_configMode(_instrumentId, modeContinuous, singleAcq, internalRefClk);
    dumpStatus(_instrumentId, status,"AcqT3Config::configMode");

    ViReal64 timeout    = 1.0;
    ViInt32  startOnArm = 0;
    status = AcqrsT3_configAcqConditions(_instrumentId, timeout, startOnArm, 0);
    dumpStatus(_instrumentId, status,"AcqT3Config::configAcqConditions");

    ViInt32  onAuxIO = 1;
    status = AcqrsT3_configMemorySwitch(_instrumentId, onAuxIO, 1, 1024*1024, 0);
    //    status = AcqrsT3_configMemorySwitch(_instrumentId, 2, 1, 1024*1024, 0);
    dumpStatus(_instrumentId, status,"AcqT3Config::configMemorySwitch");

    for(unsigned i=0; i<AcqTdcConfigType::NChannels; i++) {
      const Pds::Acqiris::TdcChannel& c = _config.channel(i);
      ViInt32 channel    = c.channel();
      ViInt32 slope      = (c.mode()<<31) | (c.slope());
      ViReal64 threshold = c.level();
      printf("Chan %d : slope %x : threshold %g\n", 
	     int(channel), int(slope), double(threshold));
      status = AcqrsT3_configChannel(_instrumentId, channel, slope, threshold, 0);
      dumpStatus(_instrumentId, status,"AcqT3Config::configChannel");
    }

    for(unsigned i=0; i<AcqTdcConfigType::NAuxIO; i++) {
      const Pds::Acqiris::TdcAuxIO& c = _config.auxio(i);
      ViInt32 channel    = c.channel();
      ViInt32 signal     = c.mode();
      ViInt32 qualifier  = c.term();
      printf("ControlIO %d : signal %x : qual %x\n", int(channel), int(signal), int(qualifier));
      status = AcqrsT3_configControlIO(_instrumentId, channel, signal, qualifier, 0.0);
      dumpStatus(_instrumentId, status,"AcqT3Config::configControlIO");
    }

    { 
      ViInt32 channel   = _config.veto().channel();
      ViInt32 signal    = _config.veto().mode();
      ViInt32 qualifier = _config.veto().term();
      printf("ControlIO %d : signal %x : qual %x\n", int(channel), int(signal), int(qualifier));
      status = AcqrsT3_configControlIO(_instrumentId, channel, signal, qualifier, 0.0);
      dumpStatus(_instrumentId, status,"AcqT3Config::configControlIO");
    }

    if (_nerror)
      _mgr.appliance().post(msg);
    else
      delete msg;

    _cfgtc.extent = sizeof(Xtc)+sizeof(AcqTdcConfigType);
    _reader.resetCount();

    if (_firstTime) {
      _reader.start();
      _firstTime = 0;
    }

    return tr;
  }
private:
  ViSession        _instrumentId;
  AcqT3Reader&     _reader;  
  AcqTdcConfigType _config;
  Xtc              _cfgtc;
  CfgClientNfs&    _cfg;
  AcqT3Manager&    _mgr;
  GenericPool      _occPool;
  unsigned         _nerror;
  unsigned         _firstTime;
};

class AcqT3EnableAction : public Action {
public:
  AcqT3EnableAction(AcqT3Reader& reader) : _reader(reader) {}
public:
  InDatagram* fire(InDatagram* dg) { return dg; }
  Transition* fire(Transition* tr) { _reader.enableAcqRead(); return tr; }
private:
  AcqT3Reader& _reader;
};

class AcqT3DisableAction : public Action {
public:
  AcqT3DisableAction(AcqT3Reader& reader) : _reader(reader) {}
public:
  InDatagram* fire(InDatagram* dg) { return dg; }
  Transition* fire(Transition* tr) { _reader.disableAcqRead(); return tr; }
private:
  AcqT3Reader& _reader;
};

Appliance& AcqT3Manager::appliance() {return _fsm;}

AcqT3Manager::AcqT3Manager(ViSession InstrumentID, AcqServer& server, CfgClientNfs& cfg) :
  _instrumentId(InstrumentID),_fsm(*new Fsm) 
{
  Task* task = new Task(TaskObject("AcqT3Readout",35)); //default priority=127 (lowest), changed to 35 => (127-35) 
  AcqT3Reader& reader = *new AcqT3Reader(_instrumentId,*this,server,task);
  reader.acqControl(new AcqT3Enable (reader),
		    new AcqT3Disable(reader));
  
  AcqT3Dma& dma = *new AcqT3Dma(reader,server,task);
  server.setDma(&dma);

  _fsm.callback(TransitionId::Map        , new AcqT3AllocAction(cfg)); 
  _fsm.callback(TransitionId::Configure  ,new AcqT3ConfigAction(_instrumentId,reader,server.client(),cfg,*this));
  _fsm.callback(TransitionId::Enable     ,new AcqT3EnableAction (reader));
  _fsm.callback(TransitionId::Disable    ,new AcqT3DisableAction(reader));
#if 0
  ViStatus status = Acqrs_calLoad(_instrumentId,AcqManager::calibPath(),1);
  if(status != VI_SUCCESS) {
    char message[256];
    Acqrs_errorMessage(_instrumentId,status,message,256);
    printf("Acqiris calibration load error: %s\n",message);
  } else {
    printf("Loaded acqiris calibration file.\n");
  }
#endif
}

AcqT3Manager::~AcqT3Manager()
{
}

