
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
#include "pdsdata/acqiris/DataDescV1.hh"

using namespace Pds;

static unsigned cpol1=0;

class AcqReader : public Routine {
public:
  AcqReader(ViSession instrumentId, AcqServer& server, Task* task) :
    _instrumentId(instrumentId),_task(task),_server(server),_count(0) {
  }
  void setConfig(const AcqConfigType& config) {
    _nbrSamples=config.horiz().nbrSamples();
    _totalSize=Acqiris::DataDescV1::totalSize(config.horiz())*
      config.nbrChannels();
  }
  void start() { _count=0; _task->call(this);}
  virtual ~AcqReader() {}
  void routine() {
    ViStatus status;
    status = AcqrsD1_acquire(_instrumentId);
    do {
      status = AcqrsD1_waitForEndOfAcquisition(_instrumentId,_nbrSamples);
      if (status != (int)ACQIRIS_ERROR_ACQ_TIMEOUT && status != VI_SUCCESS) {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
    } while (status);
    _server.headerComplete(_totalSize,_count++);
  }
private:
  ViSession  _instrumentId;
  Task*      _task;
  AcqServer& _server;
  unsigned   _count;
  unsigned   _nbrSamples;
  unsigned   _totalSize;
};

class AcqDma : public DmaEngine {
public:
  AcqDma(ViSession instrumentId, AcqReader& reader, AcqServer& server,
         Task* task) :
    DmaEngine(task),
    _instrumentId(instrumentId),_reader(reader),_server(server),_lastAcqTS(0),_count(0) {
  }
  void setConfig(AcqConfigType& config) {_config=&config;}
  void routine() {
    ViStatus status=0;
    // ### Readout the data ###
    unsigned channelMask = _config->channelMask();
    Acqiris::HorizV1& hconfig = _config->horiz();
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
    Acqiris::DataDescV1* data = (Acqiris::DataDescV1*)_destination;
    readParams.dataArraySize = data->waveformSize(hconfig);

    for (unsigned i=0;i<32;i++) {
      // note that analysis software will depend on the vertical configurations
      // being stored in the same order as the channel waveform data
      if (!(channelMask&(1<<i))) continue;
      unsigned channel = i+1;
      status = AcqrsD1_readData(_instrumentId, channel, &readParams,
                                data->waveform(hconfig),
                                (AqDataDescriptor*)data,
                                &(data->timestamp(0)));
      // for SAR mode
      //       AcqrsD1_freeBank(_instrumentId,0);

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
      data = data->nextChannel(hconfig);
    }

    _server.payloadComplete();
    _task->call(&_reader);
  }
private:
  ViSession  _instrumentId;
  AcqReader& _reader;
  AcqServer& _server;
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

class AcqL1Action : public AcqDC282Action {
public:
  AcqL1Action(ViSession instrumentId) : AcqDC282Action(instrumentId),
                                        _lastAcqTS(0),_lastEvrFid(0),
                                        _nprint(0),_checkTimestamp(0) {}
  InDatagram* fire(InDatagram* in) {
    Datagram& dg = in->datagram();
    if (!dg.xtc.damage.value()) validate(in);
    cpol1++;
    return in;
  }
  void notRunning() {_checkTimestamp=0;}
  unsigned validate(InDatagram* in) {
    Datagram& dg = in->datagram();
    Acqiris::DataDescV1& data = *(Acqiris::DataDescV1*)(dg.xtc.payload()+sizeof(Xtc));
    long long acqts = data.timestamp(0).value();
    unsigned evrfid = dg.seq.stamp().fiducials();
    unsigned long long psPerFiducial = 2777777777ULL;
    long long evrdiff = ((evrfid-_lastEvrFid)&((1<<Pds::TimeStamp::NumFiducialBits)-1))*psPerFiducial;
    long long acqdiff = acqts-_lastAcqTS;
    long long diff = abs(evrdiff - acqdiff);
    // empirical number for "fuzzy" equality comparison.
    if (_checkTimestamp && (diff>0x1000000)) {
      dg.xtc.damage.increase(Pds::Damage::OutOfSynch);
      if (_nprint++<10) {
        printf("*** Outofsynch: fiducials since last evt: %f (Evr) %f (Acq)\n",
               ((float)(evrdiff))/psPerFiducial,
               ((float)(acqdiff))/psPerFiducial);
        printf("*** Timestamps (current/previous) %d/%d (Evr(fiducials)) %lld/%lld (Acq(ps))\n",
               evrfid,_lastEvrFid,acqts,_lastAcqTS);
      }
    }
    _lastAcqTS  = acqts;
    _lastEvrFid = evrfid;
    _checkTimestamp = 1;
    return 0;
  }
private:
  unsigned long long _lastAcqTS;
  unsigned _lastEvrFid;
  unsigned _nprint;
  unsigned _checkTimestamp;
};

class AcqDisableAction : public AcqDC282Action {
public:
  AcqDisableAction(ViSession instrumentId, AcqL1Action& acql1) : AcqDC282Action(instrumentId),_acql1(acql1) {}
  Transition* fire(Transition* in) {
    printf("AcqManager received %d l1accepts\n",cpol1);
    _acql1.notRunning();
    return in;
  }
private:
  AcqL1Action& _acql1;
};

class AcqConfigAction : public AcqDC282Action {
public:
  AcqConfigAction(ViSession instrumentId, AcqReader& reader, AcqDma& dma, const Src& src) :
    AcqDC282Action(instrumentId),_reader(reader),_dma(dma), _cfgtc(_acqConfigType,src) {}
  InDatagram* fire(InDatagram* dg) {
    // insert assumes we have enough space in the input datagram
    dg->insert(_cfgtc, &_config);
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

    double sampInterval = 0.5e-6;
    double delayTime = 0.0;
    long nbrSamples =50, nbrSegments = 1;
    Acqiris::HorizV1 horizConfig(sampInterval,delayTime,nbrSamples,nbrSegments);

    long coupling = 3, bandwidth = 0;
    double fullScale = 1.0, offset = 0.0;

    long trigCoupling = 3; // DC, 50ohms
    long trigInput = -3; // External input 3.
    long trigSlope = 0;
    double trigLevel = 1000.0; // in mV, for external triggers only (otherwise % full-scale).
    Acqiris::TrigV1 trigConfig(trigCoupling, trigInput, trigSlope, trigLevel);

    long nbrConvertersPerChannel=1;
    long channelMask = 0xf;
    long  nbrBanks = 1;

    // Configure timebase
    _check(AcqrsD1_configHorizontal(_instrumentId, sampInterval, delayTime));

    // non-SAR mode
    _check(AcqrsD1_configMemory(_instrumentId, nbrSamples, nbrSegments));

    // SAR mode (currently doesn't work for reasons I don't understand - cpo)
//     _check(AcqrsD1_configMode(_instrumentId, 0, 0, 10)); // 10 = SAR mode
//     _check(AcqrsD1_configMemoryEx(_instrumentId, 0, nbrSamples, nbrSegments,
//                                   nbrBanks, 0));
//     ViUInt32 nbrSamplesHi,nbrSamplesLo;
//     ViInt32 nbrSegs,flags;
//     _check(AcqrsD1_getMemoryEx(_instrumentId, &nbrSamplesHi, &nbrSamplesLo,
//                                &nbrSegs,
//                                &nbrBanks,
//                                &flags));
//     printf("*** config %d %d %d %d %d\n",nbrSamplesHi,nbrSamplesLo,nbrSegs,
//            nbrBanks,flags);

    _check(AcqrsD1_configChannelCombination(_instrumentId,nbrConvertersPerChannel,channelMask));
    Acqiris::VertV1 vertConfig[AcqConfigType::MaxChan];
    unsigned nchan=0;
    // note that analysis software will depend on the vertical configurations
    // being stored in the same order as the channel waveform data
    for (unsigned i=0;i<32;i++) {
      if (channelMask&(1<<i)) {
        _check(AcqrsD1_configVertical(_instrumentId, i+1, fullScale, offset, coupling, bandwidth));
        new (vertConfig+nchan) Acqiris::VertV1(fullScale,offset,coupling,bandwidth);
        nchan++;
      }
    }
    // only support edge-trigger (no "TV trigger" for DC282)
    unsigned srcPattern = _generateTrigPattern(trigInput);
    _check(AcqrsD1_configTrigClass(_instrumentId, 0, srcPattern, 0, 0, 0.0, 0.0));
    _check(AcqrsD1_configTrigSource(_instrumentId, trigInput, trigCoupling, trigSlope, trigLevel, 0.0));
    _reader.start();

    new(&_config) AcqConfigType(nbrConvertersPerChannel,
                                    channelMask,
                                    nbrBanks,
                                    trigConfig,horizConfig,vertConfig);
    _cfgtc.extent = sizeof(Xtc)+sizeof(AcqConfigType);

    _dma.setConfig(_config);
    _reader.setConfig(_config);
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
  void _check(ViStatus status) {
    if(status != VI_SUCCESS)
      {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
  }
  AcqReader& _reader;
  AcqDma&    _dma;
  AcqConfigType _config;
  Xtc _cfgtc;
  Src _src;
};

Appliance& AcqManager::appliance() {return _fsm;}

AcqManager::AcqManager(ViSession InstrumentID, AcqServer& server) :
  _instrumentId(InstrumentID),_fsm(*new Fsm) {
  Task* task = new Task(TaskObject("AcqReadout"));
  AcqReader& reader = *new AcqReader(_instrumentId,server,task);
  AcqDma& dma = *new AcqDma(_instrumentId,reader,server,task);
  server.setDma(&dma);
  _fsm.callback(TransitionId::Configure,new AcqConfigAction(_instrumentId,reader,dma,server.client()));
  AcqL1Action& acql1 = *new AcqL1Action(_instrumentId);
  _fsm.callback(TransitionId::L1Accept,&acql1);
  _fsm.callback(TransitionId::Disable,new AcqDisableAction(_instrumentId,acql1));
}
