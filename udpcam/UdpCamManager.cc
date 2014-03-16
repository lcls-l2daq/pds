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
#include "pds/config/FccdConfigType.hh"
#include "UdpCamManager.hh"
#include "UdpCamServer.hh"
#include "pds/config/CfgClientNfs.hh"

using namespace Pds;

class UdpCamAction : public Action
{
 protected:
   UdpCamAction() {}
};

class UdpCamAllocAction : public Action
{
 public:
   UdpCamAllocAction(CfgClientNfs& cfg)
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

class UdpCamL1Action : public Action
{
public:
  UdpCamL1Action();
  ~UdpCamL1Action();

  InDatagram* fire(InDatagram* in);
  void validate( InDatagram* in );
  void reconfigure( void );
};

UdpCamL1Action::UdpCamL1Action()
{
   reconfigure();
}

UdpCamL1Action::~UdpCamL1Action()
{
}

void UdpCamL1Action::reconfigure( void )
{
}

InDatagram* UdpCamL1Action::fire(InDatagram* in)
{
  return in;
}

class UdpCamConfigAction : public UdpCamAction
{
   enum {MaxConfigSize=0x100000};

 public:
   UdpCamConfigAction( const Src& src0,
                        CfgClientNfs* cfg,
                        UdpCamServer* server,
                        UdpCamL1Action& L1,
                        UdpCamOccurrence* occSend )
      : _cfgtc( _fccdConfigType, src0 ),
        _cfg( cfg ),
        _server( server ),
        _L1( L1 ),
        _occSend( occSend )
  {
  _cfgtc.extent += sizeof(FccdConfigType);
  // FIXME default config needed
  _config = *new FccdConfigType;
  }

  InDatagram* events     (InDatagram* dg)
  {
    if (dg->seq.service() == TransitionId::Configure) {
      // _insert_configure(dg);
      printf( "*** %s line %d (TransitionId::Configure)\n", __PRETTY_FUNCTION__, __LINE__ );
    } else {
      printf( "*** %s line %d (NOT TransitionId::Configure)\n", __PRETTY_FUNCTION__, __LINE__ );
    }
    
    return dg;
  }
  ~UdpCamConfigAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    printf( "*** entered %s\n", __PRETTY_FUNCTION__);
    // todo: report the results of configuration to the datastream.
    // insert assumes we have enough space in the input datagram
    dg->insert(_cfgtc, &_config);

    if( _nerror ) {
      printf( "*** Found %d fccd configuration errors\n", _nerror );
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
    return dg;
  }

  Transition* fire(Transition* tr)
  {
  _nerror = 0;

  printf("UdpCamConfigAction: using fixed Fccd configuration.\n");

  _nerror = _server->configure(); // _config is not used!
  if (!_nerror) {
    _server->reset();   // clears out-of-order flag
  }
  _L1.reconfigure();

  return tr;
  }

  private:
    FccdConfigType _config;
    Xtc _cfgtc;
    Src _src;
    CfgClientNfs* _cfg;
    UdpCamServer* _server;
    unsigned _nerror;
    UdpCamL1Action& _L1;
    UdpCamOccurrence* _occSend;
};


class UdpCamUnconfigAction : public UdpCamAction {
 public:
   UdpCamUnconfigAction( UdpCamServer* server ) : _server( server ) {}
   ~UdpCamUnconfigAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _nerror ) {
         printf( "*** Found %d fccd Unconfig errors\n", _nerror );
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
  UdpCamServer* _server;
  unsigned _nerror;
};


class UdpCamEndrunAction : public UdpCamAction {
 public:
   UdpCamEndrunAction( UdpCamServer* server ) : _server( server ) {}
   ~UdpCamEndrunAction() {}

   InDatagram* fire(InDatagram* dg) {
      if( _nerror ) {
         printf( "*** Found %d fccd Endrun errors\n", _nerror );
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
  UdpCamServer* _server;
  unsigned _nerror;
};


class UdpCamBeginCalibCycleAction : public UdpCamAction
{
 public:
   UdpCamBeginCalibCycleAction( CfgClientNfs* cfg,
                                 UdpCamServer* server )
      : _cfg( cfg ),
        _server( server )
  {
  }

  ~UdpCamBeginCalibCycleAction() {}

  InDatagram* fire(InDatagram* dg)
  {
    return dg;
  }

  private:
    CfgClientNfs* _cfg;
    UdpCamServer* _server;
};


Appliance& UdpCamManager::appliance()
{
  return _fsm;
}

UdpCamManager::UdpCamManager( UdpCamServer* server,
                                CfgClientNfs* cfg )
   : _fsm(*new Fsm)
{
  printf("%s being initialized...\n", __FUNCTION__);

  _occSend = new UdpCamOccurrence(this);
  server->setOccSend(_occSend);

  UdpCamL1Action& udpCamL1 = * new UdpCamL1Action();

  const Src& src0 = server->client();

 _fsm.callback( TransitionId::Configure,
                new UdpCamConfigAction( src0, cfg, server, udpCamL1, _occSend) );
 _fsm.callback( TransitionId::Unconfigure,
                new UdpCamUnconfigAction( server ) );
 _fsm.callback( TransitionId::EndRun,
                new UdpCamEndrunAction( server ) );
 _fsm.callback( TransitionId::Map,
                new UdpCamAllocAction( *cfg ) );
 _fsm.callback( TransitionId::L1Accept, &udpCamL1 );
 _fsm.callback( TransitionId::BeginCalibCycle,
                new UdpCamBeginCalibCycleAction( cfg, server ) );
  printf("...%s initialization complete\n", __FUNCTION__);
}
