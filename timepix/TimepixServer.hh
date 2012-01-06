#ifndef __GSC16AISERVER_HH
#define __GSC16AISERVER_HH

#include <stdio.h>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/TimepixDataType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"

#include "mpxmodule.h"

namespace Pds
{
   class TimepixServer;
}

class Pds::TimepixServer
   : public EbServer,
     public EbCountSrv,
     public Routine
{
  public:
    TimepixServer( const Src& client );
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

    // Routine interface
    void routine();

    // Misc
    unsigned count() const;
    void setRelaxd(MpxModule *relaxd);
    void reset()  {_outOfOrder=0;}
    void withdraw();
    void reconnect();

    unsigned configure(const TimepixConfigType& config);
    unsigned unconfigure(void);
    enum Command {FrameAvailable=0};
    int  payloadComplete(void);
    Task* task()
      { return (_task); }

    // void go()   {_goFlag=true;}

  private:
    Xtc _xtc;
    unsigned short _count;
    // TimepixOccurrence *_occSend;
    int _outOfOrder;
    int _pfd[2];
    // bool _goFlag;
    Command    _cmd;
    Task *_task;
    MpxModule *_relaxd;
    int8_t      _readoutSpeed, _triggerMode;
    int32_t     _shutterTimeout;
    int32_t     _dac0[TPX_DACS];
    int32_t     _dac1[TPX_DACS];
    int32_t     _dac2[TPX_DACS];
    int32_t     _dac3[TPX_DACS];
};

#endif
