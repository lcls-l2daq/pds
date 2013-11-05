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
#include "pds/config/RayonixConfigType.hh"
#include "RayonixManager.hh"
#include "RayonixServer.hh"
#include "pds/config/CfgClientNfs.hh"

using namespace Pds;

/* ========================================*/
class RayonixAction : public Action
/* ========================================*/
{
protected:
  RayonixAction() {}
};

/* ========================================*/
class RayonixAllocAction : public Action
/* ========================================*/
{
public: 
  RayonixAllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}

  Transition* fire(Transition* tr)
  {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    _cfg.initialize(alloc.allocation());
    return tr;
  }

private:
  CfgClientNfs& _cfg;
};

/* ========================================*/
class RayonixL1Action : public Action
/* ========================================*/
{
public:
  RayonixL1Action();
  ~RayonixL1Action();

  InDatagram* fire(InDatagram* in);
  void validate( InDatagram* in );
  void reconfigure( void );
};


RayonixL1Action::RayonixL1Action()
{
  printf(".");
  reconfigure();
}

RayonixL1Action::~RayonixL1Action()
{
}

void RayonixL1Action::reconfigure( void )
{
   printf("Entered %s\n", __PRETTY_FUNCTION__);
}

InDatagram* RayonixL1Action::fire(InDatagram* in)
{
  return in;
}

/* =============================================*/
class RayonixConfigAction : public RayonixAction
/* =============================================*/
{
public:
  RayonixConfigAction( const Src& src0,
                       CfgClientNfs* cfg,
                       RayonixServer* server,
                       RayonixL1Action& L1,
                       RayonixOccurrence* occSend )
    : _cfgtc( _rayonixConfigType, src0 ),
      _cfg( cfg ),
      _server( server ),
      _L1( L1 ),
      _occSend( occSend )
  {
    _cfgtc.extent += sizeof(RayonixConfigType);
  }

  ~RayonixConfigAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    dg->insert(_cfgtc, &_config);
    
    if (_nerror ) {
      printf( "*** Found %d rayonix configuration errors\n", _nerror );
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }

  Transition* fire(Transition* tr)
  {
   printf("Entered %s\n", __PRETTY_FUNCTION__);
   _nerror = 0;
   int len = (*_cfg).fetch(*tr,
                           _rayonixConfigType,
                           &_config,
                           sizeof(_config) );
   if (len <= 0) {
      printf("RayonixConfigAction: failed to retrieve configuration "
             ": (%d)  %s.  Applying default. \n",
             errno,
             strerror(errno) );
      _occSend->userMessage("Rayonix: failed to retrieve configuration. \n");
      _nerror += 1;
   }
   else {
      _nerror += _server->configure( _config );
      //      _config.dump();
      if (!_nerror) {
         _server->reset();
      }
      _L1.reconfigure();
   }
   return tr;
  }

private:
   RayonixConfigType  _config;
   Xtc                _cfgtc;
   Src                _src;
   CfgClientNfs*      _cfg;
   RayonixServer*     _server;
   unsigned           _nerror;
   RayonixL1Action&   _L1;
   RayonixOccurrence* _occSend;
};


/* =================================================*/
class RayonixUnconfigAction : public RayonixAction {
/* =================================================*/
public:
  RayonixUnconfigAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixUnconfigAction() {}

  InDatagram* fire(InDatagram* dg) {
   printf("Entered %s\n", __PRETTY_FUNCTION__);
   if(_nerror ) {
      printf("*** Found %d rayonix Unconfig errors\n", _nerror );
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
   }
   return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    _nerror = 0;
    _nerror += _server->unconfigure();
    return tr;
  }

private:
  RayonixServer* _server;
  unsigned       _nerror;
};


/* =================================================*/
class RayonixEndRunAction : public RayonixAction {
/* =================================================*/ 
public:
  RayonixEndRunAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixEndRunAction() {}

  InDatagram* fire (InDatagram* dg) {
     printf("Entered %s\n", __PRETTY_FUNCTION__);
     if (_nerror ) {
        printf(" *** Found %d rayonix End run errors \n", _nerror );
        dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
     }
     return dg;
  }

   Transition* fire(Transition* tr) {
      printf("Entered %s\n", __PRETTY_FUNCTION__);
      _nerror = 0;
      _nerror += _server->endrun();
      return tr;
   }

private:
   RayonixServer*  _server;
   unsigned        _nerror;
   
};


/* ========================================================*/
class RayonixBeginCalibAction : public RayonixAction {
/* ========================================================*/
public:
  RayonixBeginCalibAction( CfgClientNfs* cfg,
                           RayonixServer* server )
    : _cfg( cfg ),
      _server( server )
  {
  }

  ~RayonixBeginCalibAction() {}

  InDatagram* fire(InDatagram* dg)
  {
     printf("Entered %s\n", __PRETTY_FUNCTION__);
     return dg;
  }

private:
  CfgClientNfs*  _cfg;
  RayonixServer* _server;
};

/* =================================================*/
class RayonixEndCalibAction : public RayonixAction {
/* =================================================*/
public:
  RayonixEndCalibAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixEndCalibAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixUnknownAction : public RayonixAction {
/* =================================================*/
public:
  RayonixUnknownAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixUnknownAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixResetAction : public RayonixAction {
/* =================================================*/
public:
  RayonixResetAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixResetAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    _server->reset();
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixUnmapAction : public RayonixAction {
/* =================================================*/
public:
  RayonixUnmapAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixUnmapAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixBeginRunAction : public RayonixAction {
/* =================================================*/
public:
  RayonixBeginRunAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixBeginRunAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixEnableAction : public RayonixAction {
/* =================================================*/
public:
  RayonixEnableAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixEnableAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

/* =================================================*/
class RayonixDisableAction : public RayonixAction {
/* =================================================*/
public:
  RayonixDisableAction( RayonixServer* server ) : _server( server ) {}
  ~RayonixDisableAction() {}

  InDatagram* fire(InDatagram* dg) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return dg;
  }

  Transition* fire(Transition* tr) {
    printf("Entered %s\n", __PRETTY_FUNCTION__);
    return tr;
  }

private:
  RayonixServer* _server;

};

Appliance& RayonixManager::appliance()
{
  return _fsm;
}

RayonixManager::RayonixManager( RayonixServer* server,
                                CfgClientNfs* cfg )
  : _fsm(*new Fsm)
{
  printf("%s being initialized...\n", __FUNCTION__);
  
  _occSend = new RayonixOccurrence(this);
  server->setOccSend(_occSend);

  RayonixL1Action& rayonixL1 = * new RayonixL1Action();
  
  const Src& src0 = server->client();

  _fsm.callback( TransitionId::Unknown,
                 new RayonixUnknownAction( server ) );
  _fsm.callback( TransitionId::Reset,
                 new RayonixResetAction( server ) );
  _fsm.callback( TransitionId::Map,
                 new RayonixAllocAction( *cfg ) );
  _fsm.callback( TransitionId::Unmap,
                 new RayonixUnmapAction( server ));
  _fsm.callback( TransitionId::Configure,
                 new RayonixConfigAction( src0, cfg, server, rayonixL1, _occSend ) );
  _fsm.callback( TransitionId::Unconfigure,
                 new RayonixUnconfigAction( server ) );
  _fsm.callback( TransitionId::BeginRun,
                 new RayonixBeginRunAction( server ) );
  _fsm.callback( TransitionId::EndRun,
                 new RayonixEndRunAction( server ) );
  _fsm.callback( TransitionId::BeginCalibCycle,
                 new RayonixBeginCalibAction( cfg, server ) );
  _fsm.callback( TransitionId::EndCalibCycle,
                 new RayonixEndCalibAction( server ) );
  _fsm.callback( TransitionId::Enable,
                 new RayonixEnableAction( server ) );
  _fsm.callback( TransitionId::Disable,
                 new RayonixDisableAction( server ) );
  _fsm.callback( TransitionId::L1Accept, &rayonixL1 );

  printf("...%s initialization complete\n", __FUNCTION__);
}






