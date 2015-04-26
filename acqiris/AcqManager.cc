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

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/SysClk.hh"
#include "pds/utility/DmaEngine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/AcqConfigType.hh"
#include "AcqManager.hh"
#include "AcqServer.hh"
#include "pdsdata/psddl/acqiris.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/CfgClientNfs.hh"
#include "acqiris/aqdrv4/AcqirisImport.h"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/vmon/VmonServerManager.hh"

#include "pds/epicstools/PVWriter.hh"
using Pds_Epics::PVWriter;

#define ACQ_TIMEOUT_MILISEC 1000

#define NTHERMS         5
#define PV_NAME_FORMAT  "%s:TEMPERATURE%d.VAL"

using namespace Pds;

static unsigned cpol1=0; 
static unsigned nprint =0;
enum {AcqUnknownTemp=999};

static long temperature(ViSession id, int module) {
  long degC = AcqUnknownTemp;
  char tempstring[32];
  sprintf(tempstring,"Temperature %d",module);
  ViStatus status = AcqrsD1_getInstrumentInfo(id, tempstring, &degC);
  if(status != VI_SUCCESS) {
    degC = AcqUnknownTemp;
    char message[256];
    AcqrsD1_errorMessage(id,status,message);
    printf("Acqiris id %u module %d temperature reading error: %s\n", (unsigned)id, module, message);
  }
  return degC;
}

static inline unsigned _waveformSize(const Acqiris::HorizV1& hconfig)
{
  return (hconfig.nbrSamples()*hconfig.nbrSegments()+Acqiris::DataDescV1Elem::_extraSize)*sizeof(short);
}

static inline int16_t* _waveform(Acqiris::DataDescV1Elem* data, const Acqiris::HorizV1& hconfig)
{
  return reinterpret_cast<int16_t*>(reinterpret_cast<char*>(data)+64+hconfig.nbrSegments()*sizeof(Acqiris::TimestampV1));
}

static inline Acqiris::TimestampV1* _timestamp(Acqiris::DataDescV1Elem* data) 
{
  return reinterpret_cast<Acqiris::TimestampV1*>(reinterpret_cast<char*>(data)+64);
}

static inline Acqiris::DataDescV1Elem* _nextChannel(Acqiris::DataDescV1Elem* data, const Acqiris::HorizV1& hconfig) 
{
  return reinterpret_cast<Acqiris::DataDescV1Elem*>(reinterpret_cast<char*>(_waveform(data,hconfig))+_waveformSize(hconfig));
}

enum Command {TemperatureUpdate=1, TemperaturePause=2, TemperatureResume=3};
enum {PvPrefixMax=40};
//
// command_t is used for intertask communication via pipes
//
typedef struct {
  Command cmd;
} command_t;

class PollRoutine : public Routine
{
public:
  PollRoutine(int pipeFd, int period, char *pAcqFlag, bool verbose, bool zealous) :
    _pipeFd(pipeFd),
    _period(period > 0 ? period : 10),
    _pAcqFlag(pAcqFlag),
    _verbose(verbose),
    _zealous(zealous)
  {
  }
  ~PollRoutine()
  {
  }
  // Routine interface
  void routine(void)
  {
    command_t sendCommand;
    sendCommand.cmd = TemperatureUpdate;

    while (1) {

      // check shutdown flag
      if ((_pAcqFlag) && (*_pAcqFlag & AcqManager::AcqFlagShutdown)) {
        goto poll_exit;
      }
      sleep(_period);
      // write to pipe
      if (::write(_pipeFd, &sendCommand, sizeof(sendCommand)) == -1) {
        perror("status pipe write");
      }

    } /* while (1) */

poll_exit:
    if (_verbose) {
      printf("*** exiting %s ***\n", __PRETTY_FUNCTION__);
    }
  }

private:
  int     _pipeFd;
  int     _period;
  char *  _pAcqFlag;
  bool    _verbose;
  bool    _zealous;
};

class StatusRoutine : public Routine
{
public:
  StatusRoutine(int pipeFd, char *pvPrefix, unsigned pvPeriod,
                char *pAcqFlag, ViSession instrumentId, bool verbose) :
    _pipeFd(pipeFd),
    _pvPeriod(pvPeriod),
    _pAcqFlag(pAcqFlag),
    _instrumentId(instrumentId),
    _verbose(verbose),
    _initialized(false),
    _paused(false)
  {
    if (pvPrefix) {
      strncpy(_pvPrefix, pvPrefix, PvPrefixMax-1);
    }
    ViStatus status = AcqrsD1_getInstrumentInfo(_instrumentId,"NbrModulesInInstrument",
                                                &_moduleCount);
    if (status != VI_SUCCESS) {
      char message[256];
      AcqrsD1_errorMessage(_instrumentId,status,message);
      printf("%s: Acqiris NbrModulesInInstrument error: %s\n", __PRETTY_FUNCTION__,  message);
      _moduleCount = 0;
    }
    if ((_moduleCount < 1) || (_moduleCount > NTHERMS)) {
      _moduleCount = 1;   // default count
    }
    if (_verbose) {
      printf("%s: module count=%u  period=%u\n", __PRETTY_FUNCTION__, _moduleCount, pvPeriod);
    }
  }
  ~StatusRoutine()
  {
    if (_initialized) {
      for (int ii=0; ii<NTHERMS; ii++) {
        delete _valu_writer[ii];
      }
    }
  }
  // Routine interface
  void routine(void)
  {
    command_t statusCommand;
    fd_set    notify_set;
    int       nfds;
    int       status;
    struct timeval  timeout;
    char      namebuf[64];

    if (ca_current_context() == NULL) {
      // Initialize Channel Access
      status = ca_context_create(ca_disable_preemptive_callback);
      if (_verbose) {
        printf("%s: Initialize Channel Access... ", __PRETTY_FUNCTION__);
      }
      if (ECA_NORMAL == status) {
        for (int ii=0; ii<NTHERMS; ii++) {
          sprintf(namebuf, PV_NAME_FORMAT, _pvPrefix, ii+1);
          _valu_writer[ii] = new PVWriter(namebuf);
        }

        // Send the requests and wait for channels to be found.
        status = ca_pend_io(1.0);
        if (ECA_NORMAL == status) {
          _initialized = true;
        } else {
          SEVCHK(status, "ca_pend_io() failed");
        }
      } else {
        SEVCHK(status, "ca_context_create() failed");
      }
      if (_verbose) {
        printf("done\n");
      }
    }

    if (ca_current_context() == NULL) {
      printf("Error: ca_current_context() returned NULL after ca_context_create()\n");
    }

    while (1) {
      while (1){
        // ca_poll(): send buffer is flushed and any outstanding CA background activity is processed
        ca_poll();
        // check shutdown flag
        if ((_pAcqFlag) && (*_pAcqFlag & AcqManager::AcqFlagShutdown)) {
          goto status_exit;
        }
        // select
        FD_ZERO(&notify_set);
        FD_SET(_pipeFd, &notify_set);
        nfds = _pipeFd + 1;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000; // 100 msec
        int rv = select(nfds, &notify_set, NULL, NULL, &timeout);
        if (rv == -1) {
          perror("select");
          sleep(1);
        } else if (rv > 0) {
          // data received on pipe
          break;
        }
      }
      // read from pipe
      int length = ::read(_pipeFd, &statusCommand, sizeof(statusCommand));

      if (length != sizeof(statusCommand)) {
        fprintf(stderr, "Error: read() returned %d in %s, expected %u\n", length, __FUNCTION__, (unsigned) sizeof(statusCommand));
      } else {
        switch (statusCommand.cmd) {
          case TemperatureUpdate:
            if (_paused) {
              break;
            }
            // record temperatures to EPICS PVs
            double temptemp;
            for (int ii = 0; ii < (int)_moduleCount; ii++) {
              temptemp = temperature(_instrumentId, ii+1);
              if (_verbose) {
                printf("  Temperature %d: %5.3g C \n", ii+1, temptemp);
              }
              *reinterpret_cast<double*>(_valu_writer[ii]->data()) = temptemp;
              _valu_writer[ii]->put();
            }
            status = ca_flush_io();   // flush I/O
            SEVCHK(status, NULL);
            break;
          case TemperaturePause:
            printf(" *** TemperaturePause ***\n");    // FIXME
            _paused = true;
            break;
          case TemperatureResume:
            printf(" *** TemperatureResume ***\n");   // FIXME
            _paused = false;
            break;
          default:
            printf("%s: unknown cmd #%d received\n", __PRETTY_FUNCTION__, statusCommand.cmd);
            break;
        }
      }
    } /* while (1) */

status_exit:
    if (_verbose) {
      printf("*** exiting %s ***\n", __PRETTY_FUNCTION__);
    }
  }

private:
  char      _pvPrefix[PvPrefixMax];
  int       _pipeFd;
  unsigned  _pvPeriod;
  char*     _pAcqFlag;
  ViSession _instrumentId;
  bool      _verbose;
  bool      _initialized;
  bool      _paused;
  PVWriter* _valu_writer[NTHERMS];
  unsigned  _moduleCount;
};

class VmonTHist {
public:
  VmonTHist(const char* title,Pds::MonGroup* group) {
    float t_ms = float(1<<20)*1.e-6;
    Pds::MonDescTH1F desc(title, "[ms]", "", 64, -0.25*t_ms, 31.75*t_ms);
    _h = new Pds::MonEntryTH1F(desc);
    group->add(_h);
  }
  ~VmonTHist() { delete _h; }
public:
  void fill(const timespec& begin, const timespec& end)
  {
    unsigned dsec(end.tv_sec - begin.tv_sec);
    if (dsec<2) {
      unsigned nsec(end.tv_nsec);
      if (dsec==1) nsec += 1000000000;
      unsigned b = (nsec-begin.tv_nsec)>>19;
      if (b < _h->desc().nbins())
        _h->addcontent(1.,b);
      else
        _h->addinfo(1.,Pds::MonEntryTH1F::Overflow);

      Pds::ClockTime clock(end.tv_sec,end.tv_nsec);
      _h->time(clock);
    }
  }
private:
  MonEntryTH1F* _h;
};  

class VmonAcq {
public:
  VmonAcq() {
    Pds::MonGroup* group = new Pds::MonGroup("Acq");
    Pds::VmonServerManager::instance()->cds().add(group);
    
    _ready_to_acq = new VmonTHist("Ready to Acq",group);
    _acq_to_dma   = new VmonTHist("Acq to DMA"  ,group);
    _dma_time     = new VmonTHist("DMA time"    ,group);
    _acq_to_ready = new VmonTHist("Acq to Ready",group);
  }    
  ~VmonAcq()
  {
    delete _ready_to_acq;
    delete _acq_to_dma;
    delete _dma_time;
    delete _acq_to_ready;
  }
public:
  void ready() 
  {
    clock_gettime(CLOCK_REALTIME, &_ts_ready); 
    _acq_to_ready->fill(_ts_acq,_ts_ready);
  }
  void acquired()
  {
    clock_gettime(CLOCK_REALTIME, &_ts_acq);
    _ready_to_acq->fill(_ts_ready,_ts_acq);
  }
  void dma_start()
  {
    clock_gettime(CLOCK_REALTIME, &_ts_start);
    _acq_to_dma->fill(_ts_acq,_ts_start);
  }
  void dma_finish()
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    _dma_time->fill(_ts_start,ts);
  }
private:
  VmonTHist* _ready_to_acq;
  VmonTHist* _acq_to_dma;
  VmonTHist* _dma_time;
  VmonTHist* _acq_to_ready;
  timespec   _ts_ready;
  timespec   _ts_acq;
  timespec   _ts_start;
};

class AcqReader : public Routine {
public:
  AcqReader(ViSession instrumentId, AcqServer& server, Task* task) :
    _instrumentId(instrumentId),_task(task),_server(server),_count(0),_acqReadEnb(false) {
    _sleepTime.tv_sec = 0;
    _sleepTime.tv_nsec = (long) 0.5e9;  //0.5 sec
  }
  void setConfig(const AcqConfigType& config) {
    _nbrSamples=config.horiz().nbrSamples();
    //    _totalSize=Acqiris::DataDescV1::totalSize(config.horiz())*config.nbrChannels();
    Acqiris::DataDescV1Elem e(0);
    _totalSize=e._sizeof(config)*config.nbrChannels();
  }
  void resetCount() {_count=0;}
  void start() {_task->call(this);}
  
  Task& task() { return *_task; }
  void acqReadEnbFlag(bool flag) {_acqReadEnb = flag;} 
  virtual ~AcqReader() {}
  void routine() {
    ViStatus status;
    char message[256];
	
    if (_acqReadEnb) {
      _vmon.ready();
      AcqrsD1_acquire(_instrumentId);		
      status = AcqrsD1_waitForEndOfAcquisition(_instrumentId,ACQ_TIMEOUT_MILISEC);
      if (status == (int)ACQIRIS_ERROR_ACQ_TIMEOUT) 
        _task->call(this);
      else {
        if (status != VI_SUCCESS) {
          AcqrsD1_errorMessage(_instrumentId,status,message);
          printf("AcqReader::Error = %s\n",message);
        }
        _vmon.acquired();
        _server.headerComplete(_totalSize,_count++);   
      }
    } else {
      nanosleep(&_sleepTime,NULL);
      _task->call(this);
    }	
  } 
  void sinkOne() {
    ViStatus status;
    char message[256];
    
    AcqrsD1_acquire(_instrumentId);		
    AcqrsD1_forceTrig(_instrumentId);		
    status = AcqrsD1_waitForEndOfAcquisition(_instrumentId,ACQ_TIMEOUT_MILISEC*1000);
    if (status == (int)ACQIRIS_ERROR_ACQ_TIMEOUT) 
      printf("AcqReader::sinkOne timeout!\n");
    else if (status != VI_SUCCESS) {
	AcqrsD1_errorMessage(_instrumentId,status,message);
	printf("AcqReader::Error = %s\n",message);
    }
    else
      printf("AcqReader::sinkOne complete\n");
    //  I don't think it's actually necessary to do the DMA
    //      _server.headerComplete(_totalSize,_count++);   
  }
public:
  VmonAcq& vmon() { return _vmon; }
private:
  ViSession   _instrumentId; 
  Task*       _task;
  AcqServer&  _server;
  unsigned    _count;
  unsigned    _nbrSamples;
  unsigned    _totalSize;
  bool        _acqReadEnb; 
  timespec    _sleepTime;
  VmonAcq     _vmon;
};


class AcqEnable : public Routine {
public:
  AcqEnable(AcqReader& reader): _acqReader(reader), _sem(Semaphore::EMPTY) { }
  ~AcqEnable() { }
  void call   () { _acqReader.task().call(this);    _sem.take(); }
  void routine() { _acqReader.acqReadEnbFlag(true); _sem.give(); }

private:
  AcqReader& _acqReader;
  Semaphore  _sem;
};

class AcqDisable : public Routine {
public:
  AcqDisable(AcqReader& reader): _acqReader(reader), _sem(Semaphore::EMPTY) { }
  ~AcqDisable() { }  
  void call   () { _acqReader.task().call(this);     _sem.take(); }
  void routine() { _acqReader.acqReadEnbFlag(false); _sem.give(); }
private:
  AcqReader& _acqReader;
  Semaphore  _sem;
};

class AcqSinkOne : public Routine {
public:
  AcqSinkOne(AcqReader& reader): _acqReader(reader), _sem(Semaphore::EMPTY) { }
  ~AcqSinkOne() { }  
  void call   () { _acqReader.task().call(this); _sem.take(); }
  void routine() { _acqReader.sinkOne(); _sem.give(); }
private:
  AcqReader& _acqReader;
  Semaphore  _sem;
};

class AcqDma : public DmaEngine {
public:
  AcqDma(ViSession instrumentId, AcqReader& reader, AcqServer& server,Task* task,Semaphore& sem) :
    DmaEngine(task),_instrumentId(instrumentId),_reader(reader),_server(server),_sem(sem),
    _lastAcqTS(0),_count(0) {}
  void setConfig(AcqConfigType& config) {_config=&config;}
  void routine() {
    _reader.vmon().dma_start();
    ViStatus status=0;
    // ### Readout the data ###
    unsigned channelMask = _config->channelMask();
    const Acqiris::HorizV1& hconfig = _config->horiz();
    unsigned nbrSegments   = hconfig.nbrSegments();
    unsigned nbrSamples    = hconfig.nbrSamples();

    AqReadParameters		readParams;

    readParams.dataType         = ReadInt16;
    // note that if the readmode changes then the structure
    // of AcqTimestamp may have to change, and the padding
    // required by driver ("extra" in AcqDataDescriptor) might
    // need to change.  Fragile! - cpo
    readParams.readMode         = ReadModeStdW;
    readParams.nbrSegments      = nbrSegments;
    readParams.flags            = 0;
    readParams.firstSampleInSeg = 0;
    readParams.firstSegment     = 0;
    readParams.segmentOffset    = 0;
    readParams.segDescArraySize = (long)sizeof(AqSegmentDescriptor) * nbrSegments;
    readParams.nbrSamplesInSeg  = nbrSamples;
    Acqiris::DataDescV1Elem* data = (Acqiris::DataDescV1Elem*)_destination;
    readParams.dataArraySize = _waveformSize(hconfig);

    for (unsigned i=0;i<32;i++) {
      // note that analysis software will depend on the vertical configurations
      // being stored in the same order as the channel waveform data
      if (!(channelMask&(1<<i))) continue;
      unsigned channel = i+1;      
      _sem.take();
      status = AcqrsD1_readData(_instrumentId, channel, &readParams,
                                _waveform(data,hconfig),
                                (AqDataDescriptor*)data, 
                                _timestamp(data));
      _sem.give();

      // for SAR mode
      // AcqrsD1_freeBank(_instrumentId,0);

      if(status != VI_SUCCESS) {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s (channel: %d)\n",message,channel);
      }
      if (data->nbrSamplesInSeg()!=nbrSamples) {
        printf("*** Received %d samples, expected %d.\n",
               data->nbrSamplesInSeg(),nbrSamples);
      }
      if (data->nbrSegments()!=nbrSegments) {
        printf("*** Received %d segments, expected %d.\n",
               data->nbrSegments(),nbrSegments);
      }
      data = _nextChannel(data,hconfig);
    }    
    _reader.vmon().dma_finish();
    _server.payloadComplete();  	
    _task->call(&_reader);
  }
private:
  ViSession  _instrumentId;
  AcqReader& _reader;
  AcqServer& _server;
  Semaphore& _sem;
  AcqConfigType* _config;
  unsigned _nbrSamples;
  long long _lastAcqTS;
  unsigned _count;
};

class AcqDC282Action : public Action {
protected:
  AcqDC282Action(ViSession instrumentId) : _instrumentId(instrumentId) {}
  ViSession _instrumentId;
};

class AcqAllocAction : public Action {
public:
  AcqAllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
  Transition* fire(Transition* tr) {
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    _cfg.initialize(alloc.allocation());
    return tr;
  }
private:
  CfgClientNfs& _cfg;
};
class AcqL1Action : public AcqDC282Action,
		    public XtcIterator {
public:
  AcqL1Action(ViSession instrumentId, AcqManager* mgr, const Src& src) :
    AcqDC282Action(instrumentId),
    _src(src),
    _lastAcqTS(0),_lastEvrFid(0),_lastEvrClockNSec(0),_initFlag(0),_evrAbsDiffNSec(0),
    _mgr(mgr),
    _occPool   (new GenericPool(sizeof(UserMessage),4)), 
    _outoforder(0) {}
  ~AcqL1Action() { delete _occPool; }

  int process(Xtc* xtc) {
    if (xtc->contains.id()==TypeId::Id_Xtc) {
      iterate(xtc);
      xtc->damage.increase(_damage);
      return 1;
    }
    else if (xtc->src == _src && xtc->contains.id()==TypeId::Id_AcqWaveform) {
      validate(xtc);
      return 0;
    }
    return 1;
  }

  InDatagram* fire(InDatagram* in) {
    Datagram& dg = in->datagram();
    _damage = 0;
    _seq    = dg.seq;
    iterate(&dg.xtc);
    return in;
  }
  void notRunning() { } 
  void reset() { _outoforder = 0; _initFlag = 0; }

  unsigned validate(Xtc* pxtc) {
    cpol1++;
    Xtc& xtc = *pxtc;
    Acqiris::DataDescV1Elem& data = *(Acqiris::DataDescV1Elem*)(xtc.payload());
    unsigned long long acqts = _timestamp(&data)->value();
    unsigned evrfid = _seq.stamp().fiducials();
    unsigned long long nsPerFiducial = 2777777ULL;
    unsigned long long evrdiff = ((evrfid > _lastEvrFid) ? (evrfid-_lastEvrFid) : (evrfid-_lastEvrFid+Pds::TimeStamp::MaxFiducials))*nsPerFiducial;    
    unsigned long long acqdiff = (long long)( (double)(acqts-_lastAcqTS)/1000.0);
    //unsigned long long diff = abs(evrdiff - acqdiff);
    
    unsigned long long evrClockSeconds = _seq.clock().seconds();
    unsigned long long evrClockNSec = _seq.clock().nanoseconds(); 
    evrClockNSec = (evrClockSeconds*1000000000) + evrClockNSec;
    unsigned long long evrClkDiffNSec = (evrClockNSec - _lastEvrClockNSec);

    //Stabilize EVR system Clock Diff time to remove high jitter present 	
    unsigned long long acqTempInt1= evrClkDiffNSec/nsPerFiducial;
    if ( (((double)evrClkDiffNSec/(double)nsPerFiducial) - (double)acqTempInt1) >= 0.5) 
      acqTempInt1++;    
    evrClkDiffNSec = acqTempInt1 * nsPerFiducial; 
    _evrAbsDiffNSec = evrdiff;
    //if time diff > 300 Sec then, use EVR Sys CLK otherwise stable fiducial count 
    if (evrClkDiffNSec > (unsigned long long)300e9)	
      _evrAbsDiffNSec = evrClkDiffNSec;
    
    unsigned long long evrAcqDiff =  abs(_evrAbsDiffNSec - acqdiff);
    // Time adaptive offset comparison to detect a dropped trigger --> Adaptive Time Offset @ 6.5 uSec/Sec rate	
    if (_initFlag && ((double)evrAcqDiff> ((double)_evrAbsDiffNSec*6.5e-3)) &&
	!_outoforder) {  
      if (nprint++<10) {
        printf("\n*** OutOfOrder: fiducials since last evt: Evr= %f  Acq= %f EvrClk= %f\n",
               ((float)(evrdiff))/(float)nsPerFiducial,((float)(acqdiff))/(float)nsPerFiducial,((float)(evrClkDiffNSec))/(float)nsPerFiducial);
        printf("*** Timestamps (current/previous) EvrFid= %d/%d AcqTimePS= %llu/%llu EvrSysClkNS= %llu/%llu \n",
               evrfid,_lastEvrFid,acqts,_lastAcqTS,evrClockNSec,_lastEvrClockNSec);		    
      }
      _outoforder=1;	  

      Pds::Occurrence* occ = new (_occPool)
        Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
      _mgr->appliance().post(occ);

      UserMessage* umsg = new (_occPool) UserMessage;
      umsg->append("Event readout error\n");
      umsg->append(DetInfo::name(static_cast<const DetInfo&>(xtc.src)));
      _mgr->appliance().post(umsg);
    }
	

    // Handle dropped event
    if (_outoforder) {
      _damage = (1<<Pds::Damage::OutOfOrder);
      xtc.damage.increase(_damage);
    }
    else {
      _lastAcqTS  = acqts;
      _lastEvrFid = evrfid;
      _lastEvrClockNSec = evrClockNSec;
    }

    //  EVR can miss the fiducial on the first event
    //  Don't check out-of-order until we get a good fiducial
    if (_initFlag==0 && evrfid!=0)
      _initFlag = 1;

    return 0;
  }
private:
  Src _src;
  unsigned long long _lastAcqTS;
  unsigned _lastEvrFid;
  long long _lastEvrClockNSec;
  unsigned _initFlag;
  unsigned long long _evrAbsDiffNSec;  
  AcqManager* _mgr;
  GenericPool* _occPool;
  unsigned _outoforder;
  unsigned _damage;
  Sequence _seq;
};

class AcqUnconfigureAction : public AcqDC282Action {
public:
  AcqUnconfigureAction(ViSession instrumentId, AcqReader& reader) : 
    AcqDC282Action(instrumentId),_acqDisable(reader) { }
  InDatagram* fire(InDatagram* in) {
    _acqDisable.call();
    return in;
  }
private:
  AcqDisable _acqDisable;
};

class AcqDisableAction : public AcqDC282Action {
public:
  AcqDisableAction(ViSession instrumentId, AcqL1Action& acql1) : AcqDC282Action(instrumentId),_acql1(acql1) {}
  InDatagram* fire(InDatagram* in) {	
    printf("AcqManager received %d l1accepts\n",cpol1);
    nprint=0;
    //_acql1.notRunning(); 
    return in;
  }
private:
  AcqL1Action& _acql1;
};

class AcqBeginCalibAction : public AcqDC282Action {
public:
  AcqBeginCalibAction(ViSession instrumentId, int pipeFd) : AcqDC282Action(instrumentId) {
    _pipeFd = pipeFd;
    _command = TemperaturePause;
  }
  InDatagram* fire(InDatagram* in) {	
    command_t sendCommand;
    sendCommand.cmd = _command;
    // write to pipe
    if (::write(_pipeFd, &sendCommand, sizeof(sendCommand)) == -1) {
      perror("status pipe write");
    }
    return in;
  }
private:
  int     _pipeFd;
  Command _command;
};

class AcqEndCalibAction : public AcqDC282Action {
public:
  AcqEndCalibAction(ViSession instrumentId, int pipeFd) : AcqDC282Action(instrumentId) {
    _pipeFd = pipeFd;
    _command = TemperatureResume;
  }
  InDatagram* fire(InDatagram* in) {	
    command_t sendCommand;
    sendCommand.cmd = _command;
    // write to pipe
    if (::write(_pipeFd, &sendCommand, sizeof(sendCommand)) == -1) {
      perror("status pipe write");
    }
    return in;
  }
private:
  int     _pipeFd;
  Command _command;
};

class AcqEnableAction : public Action {
public:
  AcqEnableAction(AcqL1Action& acql1) : _acql1(acql1) {}
  Transition* fire(Transition* in) {	
    _acql1.reset(); 
    return in;
  }
private:
  AcqL1Action& _acql1;
};

class AcqConfigAction : public AcqDC282Action {
  enum {MaxConfigSize=0x100000};
public:
  AcqConfigAction(ViSession instrumentId, AcqReader& reader, AcqDma& dma, 
		  const Src& src, CfgClientNfs& cfg,
		  AcqManager& mgr) :
    AcqDC282Action(instrumentId),_reader(reader),_dma(dma), 
    _acqEnable(reader),
    _acqSink  (reader),
    _cfgtc(_acqConfigType,src),
    _cfg(cfg), _mgr(mgr), _occPool(sizeof(UserMessage),1), _firstTime(1) {}
  ~AcqConfigAction() {}
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
    unsigned nbrModulesInInstrument;
    AcqrsD1_getInstrumentInfo(_instrumentId,"NbrModulesInInstrument",
                              &nbrModulesInInstrument);
    unsigned nbrIntTrigsPerModule;
    AcqrsD1_getInstrumentInfo(_instrumentId,"NbrInternalTriggers",
                              &nbrIntTrigsPerModule);
    unsigned nbrExtTrigsPerModule;
    AcqrsD1_getInstrumentInfo(_instrumentId,"NbrExternalTriggers",
                              &nbrExtTrigsPerModule);
    ViInt32 nbrChans;
    AcqrsD1_getNbrChannels(_instrumentId,&nbrChans);
    printf("Acqiris id 0x%x has %d/%d modules/channels with %d/%d int/ext trigger inputs\n",
           (unsigned)_instrumentId,nbrModulesInInstrument,(unsigned)nbrChans,nbrIntTrigsPerModule,nbrExtTrigsPerModule);

    // get configuration from database
    UserMessage* msg = new(&_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
    msg->append(":");

    _nerror=0;

    do {

    int len = _cfg.fetch(*tr,_acqConfigType, &_config, sizeof(_config));
    if (len <= 0) {
      printf("AcqConfigAction: failed to retrieve configuration : (%d)%s.  Applying default.\n",errno,strerror(errno));
      msg->append("Failed to open config file\n");
      _nerror++;
      continue;
    }
    //    _config.dump();

    // non-SAR mode
    if (_check("AcqrsD1_configMemory",
               AcqrsD1_configMemory(_instrumentId, _config.horiz().nbrSamples(), _config.horiz().nbrSegments()),
               msg)) _nerror++;
    uint32_t nbrSamples,nbrSegments;
    // JT 11/9/09: This is a hack to remove "type-punned" warnings.
    AcqrsD1_getMemory(_instrumentId, (ViInt32*)(void*)&nbrSamples, (ViInt32*)(void*)&nbrSegments);
    if (nbrSamples != _config.horiz().nbrSamples()) {
      printf("*** Requested %d samples, received %d\n",
             _config.horiz().nbrSamples(),nbrSamples);
      msg->append("Incorrect #samples\n");
      _nerror++;
    }
    if (nbrSegments != _config.horiz().nbrSegments()) {
      printf("*** Requested %d segments, received %d\n",
             _config.horiz().nbrSegments(),nbrSegments);
      msg->append("Incorrect #segments\n");
      _nerror++;
    }

//     SAR mode (currently doesn't work for reasons I don't understand - cpo)
//     _check(AcqrsD1_configMode(_instrumentId, 0, 0, 10)); // 10 = SAR mode
//     _check(AcqrsD1_configMemoryEx(_instrumentId, 0, nbrSamples, nbrSegments,
//                                   nbrBanks, 0));
//     ViUInt32 nbrSamplesHi,nbrSamplesLo;
//     ViInt32 nbrSegs,flags;
//     _check(AcqrsD1_getMemoryEx(_instrumentId, &nbrSamplesHi, &nbrSamplesLo,
//                                &nbrSegs,
//                                &nbrBanks,
//                                &flags));

    // use the lowest nibble of the channelmask for the channelcombination, since
    // this ends up being the same for all channels in a multi-instrument.
    uint32_t nbrConvertersPerChannel, usedChannels;
    usedChannels = _config.channelMask()&0xf;
    if (_check("AcqrsD1_configChannelCombination",
               AcqrsD1_configChannelCombination(_instrumentId,_config.nbrConvertersPerChannel(),
                                                usedChannels),
               msg)) _nerror++;
    AcqrsD1_getChannelCombination(_instrumentId, (ViInt32*)(void*)&nbrConvertersPerChannel,
                                  (ViInt32*)(void*)&usedChannels);
    if (nbrConvertersPerChannel != _config.nbrConvertersPerChannel()) {
      printf("*** Requested %d converters per channel, received %d\n",
             _config.nbrConvertersPerChannel(),nbrConvertersPerChannel);
      msg->append("Incorrect #converters\n");
      _nerror++;
    }
    if (usedChannels != (_config.channelMask()&0xf)) {
      printf("*** Requested used channels %d, received %d\n",
             _config.channelMask()&0xf,usedChannels);
      msg->append("Incorrect #channels\n");
      _nerror++;
    }

    static const double epsilon = 1.0e-15;
    unsigned nchan=0;
    double fullScale,offset;
    uint32_t coupling,bandwidth;

    for (unsigned i=0;i<32;i++) {
      uint32_t channelMask = _config.channelMask();
      if (channelMask&(1<<i)) {
        if (_check("AcqrsD1_configVertical",
                   AcqrsD1_configVertical(_instrumentId, i+1, _config.vert()[nchan].fullScale(),
                                          _config.vert()[nchan].offset(), _config.vert()[nchan].coupling(),
                                          _config.vert()[nchan].bandwidth()),
                   msg)) _nerror++;
        AcqrsD1_getVertical(_instrumentId, i+1, (ViReal64*)&fullScale,
                            (ViReal64*)&offset, (ViInt32*)(void*)&coupling,
                            (ViInt32*)(void*)&bandwidth);
        if (fabs(fullScale-_config.vert()[nchan].fullScale())>epsilon) {
          printf("*** Requested %e fullscale, received %e\n",
                 _config.vert()[nchan].fullScale(),fullScale);
	  msg->append("Incorrect scale\n");
          _nerror++;
        }
        if (fabs(offset-_config.vert()[nchan].offset())>epsilon) {
          printf("*** Requested %e fullscale, received %e\n",
                 _config.vert()[nchan].offset(),offset);
	  msg->append("Incorrect scale\n");
          _nerror++;
        }
        if (coupling != _config.vert()[nchan].coupling()) {
          printf("*** Requested coupling %d, received %d\n",
                 _config.vert()[nchan].coupling(),coupling);
	  msg->append("Incorrect coupling\n");
          _nerror++;
        }
        if (bandwidth != _config.vert()[nchan].bandwidth()) {
          printf("*** Requested bandwidth %d, received %d\n",
                 _config.vert()[nchan].bandwidth(),bandwidth);
	  msg->append("Incorrect bandwidth\n");
          _nerror++;
        }
        nchan++;
      }
    }

    // Need to configure horizontal after the vertical, because some horizontal
    // settings (like fast 8GHz sampling) require the correct channel combinations first,
    // otherwise the driver will complain.
    if (_check("AcqrsD1_configHorizontal",
               AcqrsD1_configHorizontal(_instrumentId, _config.horiz().sampInterval(),
                                        _config.horiz().delayTime()),
               msg)) _nerror++;
    double sampInterval,delayTime;
    AcqrsD1_getHorizontal(_instrumentId, (ViReal64*)&sampInterval,
                          (ViReal64*)&delayTime);
    if (fabs(sampInterval-_config.horiz().sampInterval())>epsilon) {
      printf("*** Requested %e fullscale, received %e\n",
             _config.horiz().sampInterval(),sampInterval);
      msg->append("Incorrect sample intvl\n");
      _nerror++;
    }
    if (fabs(delayTime-_config.horiz().delayTime())>epsilon) {
      printf("*** Requested %e fullscale, received %e\n",
             _config.horiz().delayTime(),delayTime);
      msg->append("Incorrect delay\n");
      _nerror++;
    }

    // only support edge-trigger (no "TV trigger" for DC282)
    unsigned srcPattern = _generateTrigPattern(_config.trig().input());
    if (_check("AcqrsD1_configTrigClass",
               AcqrsD1_configTrigClass(_instrumentId, 0, srcPattern, 0, 0, 0.0, 0.0),
               msg)) _nerror++;
    uint32_t trigClass,srcPatternOut,junk;
    double djunk;
    AcqrsD1_getTrigClass(_instrumentId, (ViInt32*)(void*)&trigClass, (ViInt32*)(void*)&srcPatternOut, (ViInt32*)(void*)&junk,
                         (ViInt32*)(void*)&junk, (ViReal64*)&djunk, (ViReal64*)&djunk);
    if (trigClass!=0) {
      printf("*** Requested trigger class 0 received %d\n",trigClass);
      msg->append("Incorrect trigger class\n");
      _nerror++;
    }
    if (srcPatternOut != srcPattern) {
      printf("*** Requested src pattern 0x%x received 0x%x\n",srcPattern,srcPatternOut);
      _nerror++;
      msg->append("Incorrect src pattern\n");
    }

    if (_check("AcqrsD1_configTrigSource",
               AcqrsD1_configTrigSource(_instrumentId, _config.trig().input(),
                                        _config.trig().coupling(), _config.trig().slope(),
                                        _config.trig().level()*1000.0, 0.0),
               msg)) _nerror++;
    uint32_t slope;
    double level;
    AcqrsD1_getTrigSource(_instrumentId, _config.trig().input(),
                          (ViInt32*)(void*)&coupling,
                          (ViInt32*)(void*)&slope, (ViReal64*)&level, &djunk);
    level /= 1000.0;
    if (fabs(level-_config.trig().level())>epsilon) {
      printf("*** Requested %e trig level, received %e\n",
             _config.trig().level(),level);
      msg->append("Incorrect trig level\n");
      _nerror++;
    }
    if (fabs(coupling!=_config.trig().coupling())>epsilon) {
      printf("*** Requested trig coupling %d, received %d\n",
             _config.trig().coupling(),coupling);
      msg->append("Incorrect trig coupling\n");
      _nerror++;
    }
    if (slope != _config.trig().slope()) {
      printf("*** Requested trig slope %d, received %d\n",
             _config.trig().slope(),slope);
      msg->append("Incorrect trig slope\n");
      _nerror++;
    }

    if (_nerror)
      _mgr.appliance().post(msg);
    else
      delete msg;

    if (_mgr._zealous) {
      char warnbuf[100];
      UserMessage* wmsg = new(&_occPool) UserMessage;
      sprintf(warnbuf, "Acqiris id %x:\n", (unsigned)_instrumentId);
      wmsg->append(warnbuf);
      wmsg->append("Temperature monitoring while data taking will cause crosstalk.\n");
      wmsg->append("Crosstalk may affect data quality.\n");
      _mgr.appliance().post(wmsg);
    }

    _cfgtc.extent = sizeof(Xtc)+sizeof(AcqConfigType);
    _dma.setConfig(_config);
    _reader.setConfig(_config);

    _acqSink.call();

    _reader.resetCount();

    _acqEnable.call();

    if (_firstTime) {
      _reader.start();
      _firstTime = 0;
    }

    } while(0);

    return tr;
  }
private:
  unsigned _generateTrigPattern(int trigChan) {
    unsigned nbrModulesInInstrument;
    AcqrsD1_getInstrumentInfo(_instrumentId,"NbrModulesInInstrument",
                              &nbrModulesInInstrument);
    unsigned srcPattern;
    if (trigChan > 0) // Internal Trigger
      {
        unsigned nbrIntTrigsPerModule;
        AcqrsD1_getInstrumentInfo(_instrumentId,"NbrInternalTriggers",
                                  &nbrIntTrigsPerModule);
        nbrIntTrigsPerModule/=nbrModulesInInstrument;
        long moduleNbr = (trigChan - 1) / nbrIntTrigsPerModule;
        long inputNbr = (trigChan - 1) % nbrIntTrigsPerModule;
        srcPattern = (moduleNbr<<16) + (0x1<<inputNbr);
      }
    else if (trigChan < 0) // External Trigger
      {
        unsigned nbrExtTrigsPerModule;
        AcqrsD1_getInstrumentInfo(_instrumentId,"NbrExternalTriggers",
                                  &nbrExtTrigsPerModule);
        nbrExtTrigsPerModule/=nbrModulesInInstrument;
        trigChan = -trigChan;
        long moduleNbr = (trigChan - 1) / nbrExtTrigsPerModule;
        long inputNbr = (trigChan - 1) % nbrExtTrigsPerModule;
        srcPattern = (moduleNbr<<16) + (0x80000000>>inputNbr);
      }
    else {
      printf("*** Error: trig chan is zero\n");
      srcPattern=0;
    }
    return srcPattern;
  }
  bool _check(const char* routine, ViStatus status, UserMessage* msg=0) {
    if(status != VI_SUCCESS)
      {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s: %s\n",routine,message);
        if (msg) msg->append(message);
        return true;
      }
    return false;
  }
private:
  AcqReader& _reader;  
  AcqDma&    _dma;
  AcqEnable  _acqEnable;
  AcqSinkOne _acqSink;
  AcqConfigType _config;
  Xtc _cfgtc;
  CfgClientNfs& _cfg;
  AcqManager& _mgr;
  GenericPool _occPool;
  unsigned _nerror;
  unsigned _firstTime;
};

Appliance& AcqManager::appliance() {return _fsm;}

unsigned AcqManager::temperature(MultiModuleNumber module) {
  ViInt32 degC = 999;
  char tempstring[32];
  sprintf(tempstring,"Temperature %d",module);
  ViStatus status = AcqrsD1_getInstrumentInfo(_instrumentId,tempstring,
                                              &degC);
  if(status != VI_SUCCESS)
    {
      char message[256];
      AcqrsD1_errorMessage(_instrumentId,status,message);
      printf("Acqiris temperature reading error: %s\n",message);
    }
  return degC;
}

const char* AcqManager::calibPath() {
  static const char* _calibPath = "/reg/g/pcds/pds/acqcalib/";
  return _calibPath;
}

AcqManager::AcqManager(ViSession InstrumentID, AcqServer& server, CfgClientNfs& cfg, Semaphore& sem, char *pvPrefix, unsigned pvPeriod, char *pAcqFlag) :
  _instrumentId(InstrumentID),_fsm(*new Fsm),_pAcqFlag(pAcqFlag)
{
  _verbose = *pAcqFlag & AcqManager::AcqFlagVerbose;
  _zealous = *pAcqFlag & AcqManager::AcqFlagZealous;
  Task* task = new Task(TaskObject("AcqReadout",35)); //default priority=127 (lowest), changed to 35 => (127-35) 
  AcqReader& reader = *new AcqReader(_instrumentId,server,task);
  
  AcqDma& dma = *new AcqDma(_instrumentId,reader,server,task,sem);
  server.setDma(&dma);
  Action* caction = new AcqConfigAction(_instrumentId,reader,dma,server.client(),cfg,*this);
  AcqL1Action& acql1 = *new AcqL1Action(_instrumentId,this,cfg.src());
  _fsm.callback(TransitionId::Configure,caction);
  _fsm.callback(TransitionId::Map, new AcqAllocAction(cfg)); 
  _fsm.callback(TransitionId::L1Accept,&acql1);
  _fsm.callback(TransitionId::Enable ,new AcqEnableAction(acql1));
  _fsm.callback(TransitionId::Disable,new AcqDisableAction(_instrumentId,acql1));
  _fsm.callback(TransitionId::Unconfigure,new AcqUnconfigureAction(_instrumentId,reader));

  Task *statusTask = (Task *)NULL;
  Task *pollTask = (Task *)NULL;
  // create status pipe
  int statusPipeFd[2];
  int err = ::pipe(statusPipeFd);
  if (err) {
    perror("pipe");
  } else {
    // allocate status task
    statusTask = new Task(TaskObject("status"));
    // allocate polling task
    pollTask = new Task(TaskObject("poll"));
    // create status thread
    StatusRoutine *statusRoutine = new StatusRoutine(statusPipeFd[0], pvPrefix, pvPeriod,
                                                     pAcqFlag, _instrumentId, _verbose);
    statusTask->call(statusRoutine);
    if (pvPeriod > 0) {
      // create polling thread to trigger temperature readout
      PollRoutine *pollRoutine = new PollRoutine(statusPipeFd[1], (int)pvPeriod, pAcqFlag, _verbose, _zealous);
      pollTask->call(pollRoutine);
      if (!_zealous) {
        // create calib callbacks to pause and resume temperature readout
        _fsm.callback(TransitionId::BeginCalibCycle, new AcqBeginCalibAction(_instrumentId, statusPipeFd[1]) );
        _fsm.callback(TransitionId::EndCalibCycle, new AcqEndCalibAction(_instrumentId, statusPipeFd[1]) );
      }
    }
  }

  ViStatus status = Acqrs_calLoad(_instrumentId,AcqManager::calibPath(),1);
  if(status != VI_SUCCESS) {
    char message[256];
    AcqrsD1_errorMessage(_instrumentId,status,message);
    printf("Acqiris calibration load error: %s\n",message);
  } else {
    printf("Loaded acqiris calibration file.\n");
  }
  /*unsigned addrLoc = 0x0212;
  unsigned* addrReg = (unsigned*) addrLoc; 
  //printf("reg = %u \n", *((unsigned*)0x0212)); 
  printf("reg = %u \n", *addrReg);
  */
}

AcqManager::~AcqManager()
{
}

