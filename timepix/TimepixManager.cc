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
#include "pds/config/CfgClientNfs.hh"

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
                        TimepixL1Action& L1,
                        TimepixOccurrence* occSend )
      : _cfgtc( _timepixConfigType, src0 ),
        _cfg( cfg ),
        _server( server ),
        _L1( L1 ),
        _occSend( occSend )
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
    _occSend->userMessage("Timepix: failed to retrieve configuration.\n");
    _nerror += 1;
  } else {
    _nerror += _server->configure( _config );
    //    _config.dump();
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
    TimepixOccurrence* _occSend;
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


class TimepixEndrunAction : public TimepixAction {
 public:
   TimepixEndrunAction( TimepixServer* server ) : _server( server ) {}
   ~TimepixEndrunAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _nerror ) {
         printf( "*** Found %d timepix Endrun errors\n", _nerror );
         dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
      return dg;
   }

   Transition* fire(Transition* tr) {
      _nerror = 0;
      _nerror += _server->endrun();
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

TimepixManager::TimepixManager( TimepixServer* server,
                                CfgClientNfs* cfg )
   : _fsm(*new Fsm)
{
  printf("%s being initialized...\n", __FUNCTION__);

  _occSend = new TimepixOccurrence(this);
  server->setOccSend(_occSend);

  TimepixL1Action& timepixL1 = * new TimepixL1Action();

  const Src& src0 = server->client();

 _fsm.callback( TransitionId::Configure,
                new TimepixConfigAction( src0, cfg, server, timepixL1, _occSend) );
 _fsm.callback( TransitionId::Unconfigure,
                new TimepixUnconfigAction( server ) );
 _fsm.callback( TransitionId::EndRun,
                new TimepixEndrunAction( server ) );
 _fsm.callback( TransitionId::Map,
                new TimepixAllocAction( *cfg ) );
 _fsm.callback( TransitionId::L1Accept, &timepixL1 );
 _fsm.callback( TransitionId::BeginCalibCycle,
                new TimepixBeginCalibCycleAction( cfg, server ) );
  printf("...%s initialization complete\n", __FUNCTION__);
}
