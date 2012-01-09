#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/TimepixConfigType.hh"
#include "TimepixManager.hh"
#include "TimepixServer.hh"
#include "pdsdata/timepix/ConfigV1.hh"
#include "pds/config/CfgClientNfs.hh"
// #include "TimepixOccurrence.hh"
#include "timepix_dev.hh"

#include "mpxmodule.h"

using namespace Pds;

class TimepixAction : public Action
{
 protected:
   TimepixAction() {}
};

class TimepixAllocAction : public Action
{
 public:
   TimepixAllocAction(CfgClientNfs& cfg)
      : _cfg(cfg) {}

   Transition* fire(Transition* tr)
   {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.initialize(alloc.allocation());
      return tr;
   }

 private:
   CfgClientNfs& _cfg;
};

class TimepixL1Action : public Action
{
public:
  TimepixL1Action();
  ~TimepixL1Action();

  InDatagram* fire(InDatagram* in);
  void validate( InDatagram* in );
  void reconfigure( void );
};

TimepixL1Action::TimepixL1Action()
{
   reconfigure();
}

TimepixL1Action::~TimepixL1Action()
{
}

void TimepixL1Action::reconfigure( void )
{
}

InDatagram* TimepixL1Action::fire(InDatagram* in)
{
  return in;
}

class TimepixConfigAction : public TimepixAction
{
   enum {MaxConfigSize=0x100000};

 public:
   TimepixConfigAction( const Src& src0,
                        CfgClientNfs* cfg,
                        TimepixServer* server,
                        TimepixL1Action& L1 )
      : _cfgtc( _timepixConfigType, src0 ),
        _cfg( cfg ),
        _server( server ),
        _L1( L1 )
  {
  _cfgtc.extent += sizeof(TimepixConfigType);
  }

  ~TimepixConfigAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    // todo: report the results of configuration to the datastream.
    // insert assumes we have enough space in the input datagram
    dg->insert(_cfgtc, &_config);

    if( _nerror ) {
      printf( "*** Found %d timepix configuration errors\n", _nerror );
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }

  Transition* fire(Transition* tr)
  {
  _nerror = 0;
  int len = (*_cfg).fetch( *tr,
                            _timepixConfigType,
                            &_config,
                            sizeof(_config) );
  if (len <= 0) {
    printf("TimepixConfigAction: failed to retrieve configuration "
           ": (%d) %s.  Applying default.\n",
           errno,
           strerror(errno) );
    // FIXME
    // _occSend->userMessage("Timepix: failed to retrieve configuration.\n");
    _nerror += 1;
  } else {
    _config.dump();
    _nerror += _server->configure( _config );
    if (!_nerror) {
      _server->reset();   // clears out-of-order flag
    }
    _L1.reconfigure();
  }

  return tr;
  }

  private:
    TimepixConfigType _config;
    Xtc _cfgtc;
    Src _src;
    CfgClientNfs* _cfg;
    TimepixServer* _server;
    unsigned _nerror;
    TimepixL1Action& _L1;
    // TimepixOccurrence* _occSend;
};


class TimepixUnconfigAction : public TimepixAction {
 public:
   TimepixUnconfigAction( TimepixServer* server ) : _server( server ) {}
   ~TimepixUnconfigAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _nerror ) {
         printf( "*** Found %d timepix Unconfig errors\n", _nerror );
         dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
      return dg;
   }

   Transition* fire(Transition* tr) {
      _nerror = 0;
      _nerror += _server->unconfigure();
      return tr;
   }

private:
  TimepixServer* _server;
  unsigned _nerror;
};


class TimepixBeginCalibCycleAction : public TimepixAction
{
 public:
   TimepixBeginCalibCycleAction( CfgClientNfs* cfg,
                                 TimepixServer* server )
      : _cfg( cfg ),
        _server( server )
  {
  }

  ~TimepixBeginCalibCycleAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    return dg;
  }

  private:
    CfgClientNfs* _cfg;
    TimepixServer* _server;
};


Appliance& TimepixManager::appliance()
{
  return _fsm;
}

static unsigned char testbuf[500000]; // FIXME new/delete

//
// warmup - Timepix initialization required for reliable startup
//
static int warmup(MpxModule *relaxd)
{
  unsigned int confReg;

  if (relaxd->readReg(MPIX2_CONF_REG_OFFSET, &confReg)) {
    fprintf(stderr, "Error: readReg(MPIX2_CONF_REG_OFFSET) failed\n");
    return (1); // ERROR
  }

  // set low clock frequency (2.5 MHz)
  confReg &= ~MPIX2_CONF_TPX_CLOCK_MASK;
  confReg |= MPIX2_CONF_TPX_CLOCK_2MHZ5;

  if (relaxd->writeReg(MPIX2_CONF_REG_OFFSET, confReg)) {
    fprintf(stderr, "Error: writeReg(MPIX2_CONF_REG_OFFSET) failed\n");
    return (1); // ERROR
  }

  // Use the shutter timer, at 100 microseconds
  // (note that the Relaxd timer has a resolution of 10 us)
  // ### Due to a bug(?) in Relaxd we need to repeat enableTimer()
  //     after each call to setDacs() (renamed setFsr())
  relaxd->enableTimer(true, 100);

  // do a couple of frame read-outs with a delay (1 sec)
  unsigned int  sz;
  int           lost_rows;

  // Open the shutter: starts the timer
  relaxd->openShutter();

  // Wait for the Relaxd timer to close the shutter
  while( !relaxd->timerExpired() ) {
    usleep(10);
  }

  // readMatrixRaw: sz=458752  lost_rows=0
  if (relaxd->readMatrixRaw(testbuf, &sz, &lost_rows) ) {
    fprintf(stderr, "Error: readMatrixRaw() failed\n");
    return (1); // ERROR
  } else if ((sz != 458752) || (lost_rows != 0)) {
    fprintf(stderr, "Error: readMatrixRaw: sz=%u  lost_rows=%d\n", sz, lost_rows);
  }

  sleep(1);   // delay 1 sec

  // Open the shutter: starts the timer
  relaxd->openShutter();

  // Wait for the Relaxd timer to close the shutter
  while( !relaxd->timerExpired() ) {
    usleep(10);
  }

  // readMatrixRaw: sz=458752  lost_rows=0
  if (relaxd->readMatrixRaw(testbuf, &sz, &lost_rows) ) {
    fprintf(stderr, "Error: readMatrixRaw() failed\n");
    return (1); // ERROR
  } else if ((sz != 458752) || (lost_rows != 0)) {
    fprintf(stderr, "Error: readMatrixRaw: sz=%u  lost_rows=%d\n", sz, lost_rows);
  }

  if (relaxd->readReg(MPIX2_CONF_REG_OFFSET, &confReg)) {
    fprintf(stderr, "Error: readReg(MPIX2_CONF_REG_OFFSET) failed\n");
    return (1); // ERROR
  }

  // set high clock frequency (100 MHz)
  confReg &= ~MPIX2_CONF_TPX_CLOCK_MASK;
  confReg |= MPIX2_CONF_TPX_CLOCK_100MHZ;

  if (relaxd->writeReg(MPIX2_CONF_REG_OFFSET, confReg)) {
    fprintf(stderr, "Error: writeReg(MPIX2_CONF_REG_OFFSET) failed\n");
    return (1); // ERROR
  }

  return (0);
}

TimepixManager::TimepixManager( TimepixServer* server,
                                CfgClientNfs* cfg )
   : _fsm(*new Fsm)
{
  printf("%s being initialized...\n", __FUNCTION__);

  // _occSend = new TimepixOccurrence(this);
  // server->setOccSend(_occSend);

  int id = 0;   // FIXME id currently fixed at 0

  // ---------------------------
  // Relaxd module instantiation
  // Access to a Relaxd module using an MpxModule class object:
  // parameter `id' determines IP-addr of the module: 192.168.33+id.175
  MpxModule *relaxd = new MpxModule( id );

  // Set verbose writing to MpxModule's logfile (default = non-verbose)
  relaxd->setLogVerbose(true);

  // only start receiving frames if init succeeds and sanity check passes
  if ((relaxd->init() == 0) && (warmup(relaxd) == 0)) {
    int ndevs = relaxd->chipCount();
    if (ndevs != Timepix::ConfigV1::ChipCount) {
      fprintf(stderr, "Error: relaxd chipCount() returned %d (expected %d)\n",
             ndevs, Timepix::ConfigV1::ChipCount);
    } else {
      // FIXME temporary printf
      printf("%s counted %d devices for module %d\n", __FUNCTION__, ndevs, id);
      // create timepix device and share with TimepixServer
      _timepix = new timepix_dev(id, relaxd);
      server->setTimepix(_timepix);
    }
  } else {
    fprintf(stderr, "Error: relaxd initialization failed\n");
  }

  TimepixL1Action& timepixL1 = * new TimepixL1Action();

  const Src& src0 = server->client();

 _fsm.callback( TransitionId::Configure,
                // new TimepixConfigAction( src0, cfg, server, timepixL1, _occSend) );
                new TimepixConfigAction( src0, cfg, server, timepixL1 ) );
 _fsm.callback( TransitionId::Unconfigure,
                new TimepixUnconfigAction( server ) );
 _fsm.callback( TransitionId::Map,
                new TimepixAllocAction( *cfg ) );
 _fsm.callback( TransitionId::L1Accept, &timepixL1 );
 _fsm.callback( TransitionId::BeginCalibCycle,
                new TimepixBeginCalibCycleAction( cfg, server ) );
  printf("%s initialization complete\n", __FUNCTION__);
}
