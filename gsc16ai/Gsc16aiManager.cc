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
#include "pds/config/Gsc16aiConfigType.hh"
#include "Gsc16aiManager.hh"
#include "Gsc16aiServer.hh"
#include "pdsdata/gsc16ai/ConfigV1.hh"
#include "pds/config/CfgClientNfs.hh"
#include "gsc16ai_dev.hh"
#include "Gsc16aiOccurrence.hh"

using namespace Pds;

class Gsc16aiAction : public Action
{
 protected:
   Gsc16aiAction() {}
};

class Gsc16aiAllocAction : public Action
{
 public:
   Gsc16aiAllocAction(CfgClientNfs& cfg)
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

class Gsc16aiL1Action : public Action
{
public:
  Gsc16aiL1Action();
  ~Gsc16aiL1Action();

  InDatagram* fire(InDatagram* in);
  void validate( InDatagram* in );
  void reconfigure( void );
};

Gsc16aiL1Action::Gsc16aiL1Action()
{
   reconfigure();
}

Gsc16aiL1Action::~Gsc16aiL1Action()
{
}

void Gsc16aiL1Action::reconfigure( void )
{
}

InDatagram* Gsc16aiL1Action::fire(InDatagram* in)
{
  return in;
}

class Gsc16aiConfigAction : public Gsc16aiAction
{
   enum {MaxConfigSize=0x100000};

 public:
   Gsc16aiConfigAction( const Src& src0,
                        CfgClientNfs* cfg,
                        Gsc16aiServer* server,
                        Gsc16aiL1Action& L1,
                        Gsc16aiOccurrence* occSend )
      : _cfgtc( _gsc16aiConfigType, src0 ),
        _cfg( cfg ),
        _server( server ),
        _L1( L1 ),
        _occSend( occSend )
  {
  _cfgtc.extent += sizeof(Gsc16aiConfigType);
  }

  ~Gsc16aiConfigAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    // todo: report the results of configuration to the datastream.
    // insert assumes we have enough space in the input datagram
    dg->insert(_cfgtc, &_config);

    if( _nerror ) {
      printf( "*** Found %d gsc16ai configuration errors\n", _nerror );
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }

  Transition* fire(Transition* tr)
  {
  _nerror = 0;
  int len = (*_cfg).fetch( *tr,
                            _gsc16aiConfigType,
                            &_config,
                            sizeof(_config) );
  if (len <= 0) {
    printf("Gsc16aiConfigAction: failed to retrieve configuration "
           ": (%d) %s.  Applying default.\n",
           errno,
           strerror(errno) );
    _occSend->userMessage("Gsc16ai: failed to retrieve configuration.\n");
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
    Gsc16aiConfigType _config;
    Xtc _cfgtc;
    Src _src;
    CfgClientNfs* _cfg;
    Gsc16aiServer* _server;
    unsigned _nerror;
    Gsc16aiL1Action& _L1;
    Gsc16aiOccurrence* _occSend;
};


class Gsc16aiUnconfigAction : public Gsc16aiAction {
 public:
   Gsc16aiUnconfigAction( Gsc16aiServer* server ) : _server( server ) {}
   ~Gsc16aiUnconfigAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _nerror ) {
         printf( "*** Found %d gsc16ai Unconfig errors\n", _nerror );
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
  Gsc16aiServer* _server;
  unsigned _nerror;
};


class Gsc16aiBeginCalibCycleAction : public Gsc16aiAction
{
 public:
   Gsc16aiBeginCalibCycleAction( CfgClientNfs* cfg,
                                 Gsc16aiServer* server )
      : _cfg( cfg ),
        _server( server )
  {
  }

  ~Gsc16aiBeginCalibCycleAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    if (_server->get_autocalibEnable()) {
      if (_server->calibrate()) {
        printf( "*** gsc16ai BeginCalib error\n");
        dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
    }
    return dg;
  }

  private:
    CfgClientNfs* _cfg;
    Gsc16aiServer* _server;
};


Appliance& Gsc16aiManager::appliance()
{
  return _fsm;
}


Gsc16aiManager::Gsc16aiManager( Gsc16aiServer* server,
                                CfgClientNfs* cfg )
   : _fsm(*new Fsm)
{
  printf("%s being initialized...\n", __FUNCTION__);

  _occSend = new Gsc16aiOccurrence(this);
  server->setOccSend(_occSend);

  gsc16ai_dev *adc = new gsc16ai_dev(AI32SSC_DEV_BASE_NAME "0");

  int ret = adc->open();
  if (ret || !adc->get_isOpen()) {
    printf("Error: %s failed to open %s\n", __FUNCTION__, adc->get_devName());
  } else {
    printf("%s opened %s\n", __FUNCTION__, adc->get_devName());
  }
  server->setAdc(adc);

  Gsc16aiL1Action& gsc16aiL1 = * new Gsc16aiL1Action();

  const Src& src0 = server->client();

 _fsm.callback( TransitionId::Configure,
                new Gsc16aiConfigAction( src0, cfg, server, gsc16aiL1, _occSend) );
 _fsm.callback( TransitionId::Unconfigure,
                new Gsc16aiUnconfigAction( server ) );
 _fsm.callback( TransitionId::Map,
                new Gsc16aiAllocAction( *cfg ) );
 _fsm.callback( TransitionId::L1Accept, &gsc16aiL1 );
 _fsm.callback( TransitionId::BeginCalibCycle,
                new Gsc16aiBeginCalibCycleAction( cfg, server ) );
}
