
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>

#include "AcqManager.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/utility/DmaEngine.hh"
#include "pds/utility/AcqServer.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"

using namespace Pds;

class AcqReader : public Routine {
public:
  AcqReader(ViSession instrumentId, AcqServer& server, Task* task) :
    _instrumentId(instrumentId),_task(task),_server(server),_count(0) {
  }
  void start() {_task->call(this);}
  virtual ~AcqReader() {}
  void routine() {
    ViStatus status;
    status = AcqrsD1_acquire(_instrumentId);
    do {
      status = AcqrsD1_waitForEndOfAcquisition(_instrumentId,NumSamples);
      if (status != (int)ACQIRIS_ERROR_ACQ_TIMEOUT && status != VI_SUCCESS) {
        char message[256];
        printf("0x%x: ",(unsigned)status);
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
    } while (status);
    Datagram dg;
    dg.xtc.alloc(NumSamples*sizeof(short));
    *(unsigned*)&dg = _count++;
    _server.headerComplete(dg);
  }
private:
  enum{NumSamples=5000};
  ViSession  _instrumentId;
  Task*      _task;
  AcqServer& _server;
  unsigned   _count;
};

class AcqDma : public DmaEngine {
public:
  AcqDma(ViSession instrumentId, AcqReader& reader, AcqServer& server,
         Task* task) :
    DmaEngine(task),
    _instrumentId(instrumentId),_reader(reader),_server(server) {
  }
  void routine() {
    ViStatus status=0;
    // ### Readout the data ###
    long channel = 1;
    long nbrSegments = 1;

    AqReadParameters		readParams;
    AqDataDescriptor		wfDesc;
    AqSegmentDescriptor		segDesc[nbrSegments];

    readParams.dataType         = ReadReal64;
    readParams.readMode         = ReadModeStdW;
    readParams.nbrSegments      = nbrSegments;
    readParams.firstSampleInSeg = 0;
    readParams.firstSegment     = 0;
    readParams.segmentOffset    = 0;
    readParams.segDescArraySize = (long)sizeof(AqSegmentDescriptor) * nbrSegments;
    readParams.nbrSamplesInSeg  = nbrSamples;
    readParams.dataArraySize    = (sizeof(double)*(nbrSamples + extra)  * (nbrSegments + 1) ); // This is the formula implemented inside the driver.

    // Read the channel 1 waveform samples 
    status = AcqrsD1_readData(_instrumentId, channel, &readParams, _destination,
                              &wfDesc, &segDesc);
	
    if(status != VI_SUCCESS)
      {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
    _server.payloadComplete();
    _task->call(&_reader);
  }
private:
  static const unsigned nbrSamples=5000;
  static const unsigned extra=32;
  ViSession  _instrumentId;
  AcqReader& _reader;
  AcqServer& _server;
};

class AcqDC282Action : public Action {
protected:
  AcqDC282Action(ViSession instrumentId) : _instrumentId(instrumentId) {}
  ViSession _instrumentId;
};

class AcqConfigAction : public AcqDC282Action {
public:
  AcqConfigAction(ViSession instrumentId, AcqReader& reader) :
    AcqDC282Action(instrumentId),_reader(reader) {}
  void fire(Transition* tr) {
    printf("Configuring acqiris %d\n",(unsigned)_instrumentId);
    double sampInterval = 10.e-6, delayTime = 0.0;
    long nbrSamples = 5000, nbrSegments = 1;
    long coupling = 3, bandwidth = 0;
    double fullScale = 1.5, offset = 0.0;
    long trigCoupling = 3; // DC, 50ohms
    long trigInput = -1; // External input 1.
    long trigSlope = 0;
    double trigLevel = 1000.0; // in mV, for external triggers only (otherwise % full-scale).

    // Configure timebase
    AcqrsD1_configHorizontal(_instrumentId, sampInterval, delayTime);
    AcqrsD1_configMemory(_instrumentId, nbrSamples, nbrSegments);

    // Configure vertical settings of channel 1
    AcqrsD1_configVertical(_instrumentId, 1, fullScale, offset, coupling, bandwidth);

    // Configure edge trigger on channel 1
//     AcqrsD1_configTrigClass(_instrumentId, 0, 0x00000001, 0, 0, 0.0, 0.0);
    
    ViStatus status;
    status = AcqrsD1_configTrigClass(_instrumentId, 0, 0x80000000, 0, 0, 0.0, 0.0);
    if(status != VI_SUCCESS)
      {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
    // Configure the trigger conditions of channel 1 internal trigger
    AcqrsD1_configTrigSource(_instrumentId, trigInput, trigCoupling, trigSlope, trigLevel, 0.0);
    if(status != VI_SUCCESS)
      {
        char message[256];
        AcqrsD1_errorMessage(_instrumentId,status,message);
        printf("%s\n",message);
      }
    _reader.start();
  }
private:
  AcqReader& _reader;
};

Appliance& AcqManager::appliance() {return _fsm;}

AcqManager::AcqManager(ViSession InstrumentID, AcqServer& server) :
  _instrumentId(InstrumentID),_fsm(*new Fsm) {
  Task* task = new Task(TaskObject("AcqReadout"));
  AcqReader& reader = *new AcqReader(_instrumentId,server,task);
  AcqDma& dma = *new AcqDma(_instrumentId,reader,server,task);
  server.setDma(&dma);
  _fsm.callback(TransitionId::Configure,new AcqConfigAction(_instrumentId,reader));
}
