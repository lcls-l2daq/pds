#ifndef __TIMEPIXSERVER_HH
#define __TIMEPIXSERVER_HH

#include <stdio.h>
#include <time.h>
#include <vector>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/TimepixDataType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"

#include "timepix_dev.hh"

#include "mpxmodule.h"

#define TIMEPIX_DEBUG_PROFILE         0x00000001
#define TIMEPIX_DEBUG_NONBLOCK        0x00000002
#define TIMEPIX_DEBUG_CONVERT         0x00000004

namespace Pds
{
   class TimepixServer;
}

class Pds::TimepixServer
   : public EbServer,
     public EbCountSrv
{
  public:
    TimepixServer(const Src& client, unsigned moduleId, unsigned verbosity, unsigned debug);
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
    void reset()  {_outOfOrder=0;}
    void withdraw();
    void reconnect();

    unsigned configure(const TimepixConfigType& config);
    unsigned unconfigure(void);
    enum Command {FrameAvailable=0, TriggerConfigured, TriggerNotConfigured};
    enum {BufferDepth=64};

    void setTimepix(timepix_dev *timepix);
//  void setOccSend(TimepixOccurrence* occSend);

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
      unsigned char         _rawData[TIMEPIX_RAW_DATA_BYTES];
      Pds::Timepix::DataV1  _header;
   // unsigned char         _pixelData[TIMEPIX_DECODED_DATA_BYTES];
      int16_t               _pixelData[TIMEPIX_DECODED_DATA_BYTES / sizeof(int16_t)];
    };

    int payloadComplete(vector<BufferElement>::iterator buf_iter);

    //
    // command_t is used for intertask communication via pipes
    //
    typedef struct {
      Command cmd;
      uint16_t frameCounter;
      vector<BufferElement>::iterator buf_iter;
    } command_t;

    class ReadRoutine : public Routine
    {
    public:
      ReadRoutine(TimepixServer *server, int writeFd) :
        _server(server),
        _writeFd(writeFd)   {}
      ~ReadRoutine()        {}
      // Routine interface
      void routine(void);

    private:
      TimepixServer *_server;
      int _writeFd;
    };

    class DecodeRoutine : public Routine
    {
    public:
      DecodeRoutine(TimepixServer *server, int readFd) :
        _server(server),
        _readFd(readFd)     {}
      ~DecodeRoutine()      {}
      // Routine interface
      void routine(void);

    private:
      TimepixServer *_server;
      int _readFd;
    };

    //
    // private variables
    //

    Xtc _xtc;
    unsigned    _count;
    // TimepixOccurrence *_occSend;
    unsigned    _moduleId;
    unsigned    _verbosity;
    unsigned    _debug;
    int _outOfOrder;
    bool _triggerConfigured;
    timepix_dev *_timepix;
    int _rawPipeFd[2];
    int _completedPipeFd[2];
    vector<BufferElement>*   _buffer;
    Task *_readTask;
    ReadRoutine *_readRoutine;
    Task *_decodeTask;
    DecodeRoutine *_decodeRoutine;
    int8_t      _readoutSpeed, _triggerMode;
    int32_t     _shutterTimeout;
    int32_t     _dac0[TPX_DACS];
    int32_t     _dac1[TPX_DACS];
    int32_t     _dac2[TPX_DACS];
    int32_t     _dac3[TPX_DACS];
};

#endif
