#ifndef __GSC16AISERVER_HH
#define __GSC16AISERVER_HH

#include <stdio.h>
#include <vector>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/Gsc16aiConfigType.hh"
#include "pds/config/Gsc16aiDataType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"

#include "gsc16ai_dev.hh"
#include "Gsc16aiOccurrence.hh"

using namespace std;

namespace Pds
{
   class Gsc16aiServer;
}

class Pds::Gsc16aiServer
   : public EbServer,
     public EbCountSrv,
     public Routine
{
  public:
    Gsc16aiServer( const Src& client );
    virtual ~Gsc16aiServer() {}
      
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

    // Routine interface
    void routine();

    // Misc
    unsigned count() const;
    void setAdc(gsc16ai_dev* adc);
    void reset()  {_outOfOrder=0;}
    void setOccSend(Gsc16aiOccurrence* occSend);
    bool get_autocalibEnable();
    int calibrate();
    void withdraw();
    void reconnect();

    unsigned configure(const Gsc16aiConfigType& config);
    unsigned unconfigure(void);
    enum Command {Payload=0};
    enum {BufferDepth=64};
    Task* task()
      { return (_task); }

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
      bool     _full;
      uint16_t _timestamp[3];
      uint16_t _channelValue[Gsc16ai::NumChannels * 2];
    };

    int payloadComplete(vector<BufferElement>::iterator buf_iter);

    //
    // command_t is used for intertask communication via pipes
    //
    typedef struct {
      Command cmd;
      vector<BufferElement>::iterator buf_iter;
    } command_t;


    Xtc _xtc;
    unsigned _count;
    gsc16ai_dev *_adc;
    Gsc16aiOccurrence *_occSend;
    int _outOfOrder;
    int _pfd[2];
    // Command    _cmd;
    vector<BufferElement>*   _buffer;
    bool     _shutdownFlag;
    Task *_task;
    Gsc16aiConfigType _config;
    uint16_t _firstChan;
    uint16_t _lastChan;
    bool     _timeTagEnable;
    bool     _autocalibEnable;
};

#endif
