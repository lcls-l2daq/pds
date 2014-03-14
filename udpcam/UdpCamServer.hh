#ifndef __UDPCAMSERVER_HH
#define __UDPCAMSERVER_HH

#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <vector>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/camera/FrameType.hh"      // Frame V1

#include "UdpCamOccurrence.hh"

#define UDPCAM_DEBUG_IGNORE_FRAMECOUNT  0x00000010

#define UDP_RCVBUF_SIZE     (64*1024*1024)

namespace Pds
{
   class UdpCamServer;
}

class Pds::UdpCamServer
   : public EbServer,
     public EbCountSrv
{
  public:
    UdpCamServer(const Src& client, unsigned verbosity, int dataPort, unsigned debug);
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

  private:

    //
    // private classes
    //


    //
    // private variables
    //

    Xtc _xtc;
    Xtc _xtcDamaged;
    unsigned    _count;
    bool        _resetHwCount;
    unsigned    _countOffset;
    UdpCamOccurrence *_occSend;
    unsigned    _verbosity;
    int         _dataPort;
    unsigned    _debug;
    int         _outOfOrder;
    int         _shutdownFlag;
    int         _dataFd;
    unsigned    _currPacket;
    unsigned    _prevPacket;
    bool        _frameStarted;
    unsigned    _packetCount;
};

#endif
