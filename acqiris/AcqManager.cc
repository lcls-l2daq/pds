
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
#include "pdsdata/acqiris/ConfigV1.hh"
#include "AcqManager.hh"
#include "AcqServer.hh"
#include "AcqData.hh"

using namespace Pds;

static unsigned cpol1=0;

class AcqReader : public Routine {
public:
  AcqReader(ViSession instrumentId, AcqServer& server, Task* task) :
    _instrumentId(instrumentId),_task(task),_server(server),_count(0) {
  }
  void start() { _count=0; _task->call(this);}
  void setSamples(unsigned nbrSamples) {_nbrSamples=nbrSamples;}
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
    // kludge the size for now.
    _server.headerComplete(sizeof(AcqData)+sizeof(AcqTimestamp)+_nbrSamples*sizeof(short),_count++);
  }
private:
  ViSession  _instrumentId;
  Task*      _task;
  AcqServer& _server;
  unsigned   _count;
  unsigned   _nbrSamples;
};

class AcqDma : public DmaEngine {
public:
  AcqDma(ViSession instrumentId, AcqReader& reader, AcqServer& server,
         Task* task) :
    DmaEngine(task),
    _instrumentId(instrumentId),_reader(reader),_server(server),_lastAcqTS(0),_count(0) {
  }
  void setConfig(Acqiris::ConfigV1& config) {_config=&config;}
  void routine() {
    ViStatus status=0;
    // ### Readout the data ###
    long channelMask = _config->channelMask();
    long nbrSegments = _config->horizConfig().nbrSegments();
    long nbrSamples  = _config->horizConfig().nbrSamples();

    AqReadParameters		readParams;

    //     readParams.dataType         = ReadReal64;
    readParams.dataType         = ReadInt16;
    readParams.readMode         = ReadModeStdW;
    readParams.nbrSegments      = nbrSegments;
    readParams.flags            = 0;
    readParams.firstSampleInSeg = 0;
    readParams.firstSegment     = 0;
    readParams.segmentOffset    = 0;
    readParams.segDescArraySize = (long)sizeof(AqSegmentDescriptor) * nbrSegments;
    readParams.nbrSamplesInSeg  = nbrSamples;
    AcqData* data = (AcqData*)_destination;
    readParams.dataArraySize = 
      (char*)(data->nextChannel(nbrSegments,nbrSamples))-
      (char*)(data->waveforms(nbrSegments));

    for (unsigned i=0;i<32;i++) {
      if (!(channelMask&(1<<i))) continue;
      unsigned channel = i+1;
//       printf("desc %p, segdesc %p, wf %p, nextch %p, dsize 0x%x\n",
//              &(data->desc()),&(data->timestamp(0).desc()),
//              data->waveforms(nbrSegments),
//              data->nextChannel(nbrSegments,nbrSamples),
//              readParams.dataArraySize);
      status = AcqrsD1_readData(_instrumentId, channel, &readParams,
                                data->waveforms(nbrSegments),
                                &(data->desc()),
                                &(data->timestamp(0).desc()));
      // for SAR mode
      //       AcqrsD1_freeBank(_instrumentId,0);

      if(status != VI_SUCCESS) {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s (channel: %d)\n",message,channel);
      }
      data = data->nextChannel(nbrSegments,nbrSamples);
    }

    _server.payloadComplete();
    _task->call(&_reader);
  }
private:
  ViSession  _instrumentId;
  AcqReader& _reader;
  AcqServer& _server;
  Acqiris::ConfigV1* _config;
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
    //     Datagram& dg = in->datagram();
    //     if (!dg.xtc.damage.value()) validate(in);
    cpol1++;
    return in;
  }
  void notRunning() {_checkTimestamp=0;}
  unsigned validate(InDatagram* in) {
    Datagram& dg = in->datagram();
    AcqData& data = *(AcqData*)(dg.xtc.payload()+sizeof(Xtc));
    long long acqts = data.timestamp(0).value();
    unsigned evrfid = dg.seq.high();
    unsigned long long psPerFiducial = 2777777777ULL;
    long long evrdiff = ((evrfid-_lastEvrFid)&((1<<Pds::Sequence::NumFiducialBits)-1))*psPerFiducial;
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
  AcqConfigAction(ViSession instrumentId, AcqReader& reader, AcqDma& dma) :
    AcqDC282Action(instrumentId),_reader(reader),_dma(dma),
    _pool(sizeof(Acqiris::ConfigV1)+sizeof(CDatagram),1) {}
  InDatagram* fire(InDatagram* input) {
    CDatagram& output = *new (&_pool)CDatagram(input->datagram(),
                                               _config.typeId(),
                                               input->datagram().xtc.src);
    void* payloadAddr = output.dg().xtc.alloc(sizeof(Acqiris::ConfigV1));
    memcpy(payloadAddr,&_config,sizeof(Acqiris::ConfigV1));
    return &output;
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

    double sampInterval = 0.125e-9;
    double delayTime = 0.0;
    long nbrSamples =5000, nbrSegments = 1;
    Acqiris::HorizConfigV1 horizConfig(sampInterval,delayTime,nbrSamples,nbrSegments);

    long coupling = 3, bandwidth = 0;
    double fullScale = 1.5, offset = 0.0;
    Acqiris::VertConfigV1 vertConfig[Acqiris::ConfigV1::MaxChan] = {
      Acqiris::VertConfigV1(fullScale,offset,coupling,bandwidth)
    };

    long trigCoupling = 3; // DC, 50ohms
    long trigInput = -3; // External input 3.
    long trigSlope = 0;
    double trigLevel = 1000.0; // in mV, for external triggers only (otherwise % full-scale).
    Acqiris::TrigConfigV1 trigConfig(trigCoupling, trigInput, trigSlope, trigLevel);

    long nbrConvertersPerChannel=2;
    long channelMask = 0x6;
    long  nbrBanks = 1;

    new(&_config) Acqiris::ConfigV1(nbrConvertersPerChannel,
                                    channelMask,
                                    nbrBanks,
                                    trigConfig,horizConfig,vertConfig);

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
    for (unsigned i=0;i<32;i++) {
      if (channelMask&(1<<i))
        _check(AcqrsD1_configVertical(_instrumentId, i+1, fullScale, offset, coupling, bandwidth));
    }
    // only support edge-trigger (no "TV trigger" for DC282)
    unsigned srcPattern = _generateTrigPattern(trigInput);
    _check(AcqrsD1_configTrigClass(_instrumentId, 0, srcPattern, 0, 0, 0.0, 0.0));
    _check(AcqrsD1_configTrigSource(_instrumentId, trigInput, trigCoupling, trigSlope, trigLevel, 0.0));
    _reader.start();
    _reader.setSamples(nbrSamples);
    _dma.setConfig(_config);
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
  Acqiris::ConfigV1 _config;
  GenericPool _pool;
};

Appliance& AcqManager::appliance() {return _fsm;}

AcqManager::AcqManager(ViSession InstrumentID, AcqServer& server) :
  _instrumentId(InstrumentID),_fsm(*new Fsm) {
  Task* task = new Task(TaskObject("AcqReadout"));
  AcqReader& reader = *new AcqReader(_instrumentId,server,task);
  AcqDma& dma = *new AcqDma(_instrumentId,reader,server,task);
  server.setDma(&dma);
  _fsm.callback(TransitionId::Configure,new AcqConfigAction(_instrumentId,reader,dma));
  AcqL1Action& acql1 = *new AcqL1Action(_instrumentId);
  _fsm.callback(TransitionId::L1Accept,&acql1);
  _fsm.callback(TransitionId::Disable,new AcqDisableAction(_instrumentId,acql1));
}
