#ifndef __TIMEPIXSERVER_HH
#define __TIMEPIXSERVER_HH

#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <vector>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/TimepixDataType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"

#include "timepix_dev.hh"
#include "TimepixOccurrence.hh"

#include "mpxmodule.h"

#define TIMEPIX_DEBUG_PROFILE             0x00000001
#define TIMEPIX_DEBUG_TIMECHECK           0x00000002
#define TIMEPIX_DEBUG_NOCONVERT           0x00000004
#define TIMEPIX_DEBUG_KEEP_ERR_PIXELS     0x00000008
#define TIMEPIX_DEBUG_IGNORE_FRAMECOUNT   0x00000010

namespace Pds
{
   class TimepixServer;
}

class Pds::TimepixServer
   : public EbServer,
     public EbCountSrv
{
  public:
    TimepixServer(const Src& client, unsigned moduleId, unsigned verbosity, unsigned debug, char *threshFile,
                  char *imageFileName, int cpu0, int cpu1);
    virtual ~TimepixServer() {}
      
    //  Eb interface
    void       dump ( int detail ) const {}
    bool       isValued( void ) const    { return true; }
    const Src& client( void ) const      { return _xtc.src; }

    //  EbSegment interface
    const Xtc& xtc( void ) const    { return _xtc; }
    unsigned   offset( void ) const { return sizeof(Xtc); }
    unsigned   length() const       { return _xtc.extent; }

    //  Eb-key interface
    EbServerDeclare;

    //  Server interface
    int pend( int flag = 0 ) { return -1; }
    int fetch( ZcpFragment& , int flags ) { return 0; }
    int fetch( char* payload, int flags );
    // Misc
    unsigned count() const;
    unsigned moduleId() const;
    unsigned verbosity() const;
    unsigned debug() const;
    uint8_t* pixelsCfg();
    void reset()  {_outOfOrder=0;}
    void withdraw();
    void reconnect();

    unsigned configure(TimepixConfigType& config);
    unsigned unconfigure(void);
    unsigned endrun(void);
    enum Command {FrameAvailable=0, TriggerConfigured, TriggerNotConfigured, CommandShutdown};
    enum TaskState {TaskShutdown=0, TaskInit, TaskWaitFrame, TaskReadFrame, TaskWaitConfigure, TaskReadPipe};

    enum {BufferDepth=8};
    enum {ReadThreads=2};

    void setTimepix(timepix_dev *timepix);
    void setOccSend(TimepixOccurrence* occSend);
    void shutdown();

  private:

    //
    // private classes
    //

    class BufferElement
    {
    public:
      BufferElement() :
        _full(false)
      {}

      bool                  _full;
      unsigned char         _rawData[Pds::Timepix::DataV1::RawDataBytes];
      Pds::Timepix::DataV1  _header;
      int16_t               _pixelData[Pds::Timepix::DataV1::DecodedDataBytes / sizeof(int16_t)];
    };

    int payloadComplete(vector<BufferElement>::iterator buf_iter, bool missedTrigger);

    //
    // command_t is used for intertask communication via pipes
    //
    typedef struct {
      Command cmd;
      uint16_t frameCounter;
      bool missedTrigger;
      vector<BufferElement>::iterator buf_iter;
    } command_t;

    class ReadRoutine : public Routine
    {
    public:
      ReadRoutine(TimepixServer *server, int taskNum, int cpuAffinity) :
        _server(server),
        _taskNum(taskNum),
        _cpuAffinity(cpuAffinity)   {}
      ~ReadRoutine()        {}
      // Routine interface
      void routine(void);

    private:
      TimepixServer *_server;
      int _taskNum;
      int _cpuAffinity;
    };

    //
    // private variables
    //

    Xtc _xtc;
    Xtc _xtcDamaged;
    unsigned    _count;
    unsigned    _missedTriggerCount;
    bool        _resetHwCount;
    unsigned    _countOffset;
    TimepixOccurrence *_occSend;
    unsigned    _expectedDiff;
    int         _resetTimestampCount;
    unsigned    _moduleId;
    unsigned    _verbosity;
    unsigned    _debug;
    int         _outOfOrder;
    uint16_t    _badFrame;
    uint16_t    _uglyFrame;
    bool _triggerConfigured;
    bool _profileCollected;
    timepix_dev *_timepix;
    int _completedPipeFd[2];
    vector<BufferElement>*   _buffer[ReadThreads];
    ReadRoutine *            _readRoutine[ReadThreads];
    Task *                   _readTask[ReadThreads];
    int _shutdownFlag;
    uint8_t *   _pixelsCfg;
    int8_t      _readoutSpeed, _triggerMode;
    int32_t     _timepixSpeed;  // replaced _shutterTimeout
    int32_t     _dac0[TPX_DACS];
    int32_t     _dac1[TPX_DACS];
    int32_t     _dac2[TPX_DACS];
    int32_t     _dac3[TPX_DACS];
    char *      _threshFile;
    int16_t *   _testData;
    MpxModule * _relaxd;
    int         _cpu0;
    int         _cpu1;
    Semaphore * _readTaskMutex;
};

#endif
