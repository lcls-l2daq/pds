#ifndef __UDPCAMSERVER_HH
#define __UDPCAMSERVER_HH

#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <vector>

#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
// #include "pds/service/Semaphore.hh"

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/camera/FrameType.hh"      // Frame V1
#include "pds/config/FrameFexConfigType.hh"

#include "UdpCamOccurrence.hh"
  
#define UDPCAM_DEBUG_IGNORE_FRAMECOUNT  0x00000010
#define UDPCAM_DEBUG_IGNORE_PACKET_CNT  0x00000020
#define UDPCAM_DEBUG_NO_REORDER         0x00000040

#define UDP_RCVBUF_SIZE     (64*1024*1024)
#define FRAMEBUF_SIZE       (2*1024*1024)

using std::vector;

namespace Pds
{
   class UdpCamServer;
}

class Pds::UdpCamServer
   : public EbServer,
     public EbCountSrv
{
  public:
    UdpCamServer(const Src& client, unsigned verbosity, int dataPort, unsigned debug, int cpu0);
    virtual ~UdpCamServer() {}
      
    //  Eb interface
    void       dump ( int detail ) const {}
    bool       isValued( void ) const    { return true; }
    const Src& client( void ) const      { return _xtc.src; }

    //  EbSegment interface
    const Xtc& xtc( void ) const    { return _xtc; }
    unsigned   offset( void ) const { return sizeof(Xtc); }
    unsigned   length() const       { return _xtc.extent; }
//  bool       more  () const       { return true; }

    //  Eb-key interface
    EbServerDeclare;

    //  Server interface
    int pend( int flag = 0 ) { return -1; }
    int fetch( ZcpFragment& , int flags ) { return 0; }
    int fetch( char* payload, int flags );
    // Misc
    unsigned count() const;
    unsigned verbosity() const;
    unsigned debug() const;
    void reset()  {_outOfOrder=0;}
    void withdraw();
    void reconnect();

    unsigned configure(void);
    unsigned unconfigure(void);
    unsigned endrun(void);

    void setOccSend(UdpCamOccurrence* occSend);
    void shutdown();

    enum {LastPacketIndex = 226};
    enum Command {FrameAvailable=0, TriggerConfigured, TriggerNotConfigured, CommandShutdown};
    enum {BufferCount=64};
    enum {PacketDataSize=8192};

  private:

    typedef struct {
      char packetIndex;
      char f0;
      char f1;
      char f2;
      char f3;
      char f4;
      uint16_t frameIndex;
    } cmd_t;

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
      bool                  _damaged;
      cmd_t                 _header;
      unsigned char         _rawData[1024 * 1024 * 2];
      int16_t               _pixelData[960 * 960];
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
      ReadRoutine(UdpCamServer *server, int taskNum, int cpuAffinity) :
        _server(server),
        _taskNum(taskNum),
        _cpuAffinity(cpuAffinity)   {}
      ~ReadRoutine()        {}
      // Routine interface
      void routine(void);

    private:
      UdpCamServer *_server;
      int _taskNum;
      int _cpuAffinity;
    };


    //
    // private variables
    //

    Xtc _xtc;
    Xtc _xtcDamaged;
    Xtc _xtcUndamaged;
    unsigned    _count;
    bool        _resetHwCount;
    unsigned    _countOffset;
    UdpCamOccurrence *_occSend;
    unsigned    _verbosity;
    int         _dataPort;
    unsigned    _debug;
    int         _outOfOrder;
    int         _dataFd;
    unsigned    _currPacket;
    unsigned    _prevPacket;
    bool        _frameStarted;
    unsigned    _goodPacketCount;
    int         _shutdownFlag;
    int _completedPipeFd[2];
    vector<BufferElement>*   _frameBuffer;
    ReadRoutine *            _readRoutine;
    Task *                   _readTask;
    int         _cpu0;

    enum { mapLength = 192};
    uint16_t _mapCol[mapLength];// new?
    uint16_t _mapCric[mapLength];
    uint16_t _mapAddr[mapLength];
    uint16_t _chanMap[mapLength];
    uint16_t _topBot[mapLength];

};

#endif
