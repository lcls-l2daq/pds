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
#include "pds/config/EncoderConfigType.hh"
#include "pci3e_dev.hh"
#include "EncoderManager.hh"
#include "EncoderServer.hh"
#include "pdsdata/encoder/DataV1.hh"
#include "pdsdata/encoder/ConfigV1.hh"
#include "pds/config/CfgClientNfs.hh"
// FIXME: Only for initial bringup - when the configuration database
// is in place, this isn't needed.
#include "driver/pci3e.h"

using namespace Pds;

class EncoderAction : public Action
{
 protected:
   EncoderAction() {}
};

class EncoderAllocAction : public Action
{
 public:
   EncoderAllocAction(CfgClientNfs& cfg)
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

class EncoderL1Action : public Action
{
 public:
   EncoderL1Action() {}

   InDatagram* fire(InDatagram* in)
   {
      Datagram& dg = in->datagram();
      if (int d = dg.xtc.damage.value()) {
         printf("Damage 0x%x\n",d);
      }
      return in;
   }
};

class EncoderConfigAction : public EncoderAction
{
   enum {MaxConfigSize=0x100000};

 public:
   EncoderConfigAction( const Src& src0,
                      CfgClientNfs** cfg,
                      EncoderServer* server[],
                      int nServers )
      : _cfgtc( _encoderConfigType, src0 ),
        _cfg( cfg ),
        _server( server ),
        _nServers( nServers ) {}
   ~EncoderConfigAction() {}

   InDatagram* fire(InDatagram* dg)
   {
      // todo: report the results of configuration to the datastream.
      // insert assumes we have enough space in the input datagram
      dg->insert(_cfgtc, &_config);
      if( _nerror )
      {
         printf( "*** Found %d ipimb configuration errors\n", _nerror );
         dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
      return dg;
   }

   Transition* fire(Transition* tr)
   {
      // todo: get configuration from database.
      // program/validate register settings
      //    int len = _cfg.fetch(*tr,_ipimbConfigType, &_config, sizeof(_config));
      int len = 1;

      if (len <= 0)
      {
         printf("EncoderConfigAction: failed to retrieve configuration "
                ": (%d)%s.  Applying default.\n",
                errno,
                strerror(errno) );
      }

      // FIXME - Remove after config db is working.
      new (&_config) Encoder::ConfigV1( 0, // channel 0
                                        COUNT_MODE_WRAP_FULL,
                                        QUAD_MODE_X1,
                                        0, // Input 0
                                        1 ); // trigger on rising edge
      _config.dump();

      _nerror = 0;
      for (unsigned i = 0; i < _nServers; i++)
      {
         // int len = *_cfg[i].fetch(*tr,_ipimbConfigType, &_config, sizeof(_config));
         _nerror += _server[i]->configure( _config );
      }

      return tr;
   }

 private:
   EncoderConfigType _config;
   Xtc _cfgtc;
   Src _src;
   CfgClientNfs** _cfg;
   EncoderServer** _server;
   unsigned _nerror;
   unsigned _nServers;
};


EncoderManager::EncoderManager( EncoderServer* server[],
                                unsigned nServers,
                                CfgClientNfs** cfg )
   : _fsm(*new Fsm),
     _nServers(nServers)
{
   char portName[12];

   printf("initialized with %d servers\n", _nServers);

   for (unsigned i = 0; i < _nServers; i++)
   {
      sprintf( portName, "/dev/pci3e%d", i );
      PCI3E_dev* encoder = new PCI3E_dev( portName );
      server[i]->setEncoder( encoder );
   }

   const Src& src0 = server[_nServers-1]->client();
   Action* caction = new EncoderConfigAction( src0,
                                              cfg,
                                              server,
                                              _nServers);
   _fsm.callback( TransitionId::Configure, caction );

   EncoderL1Action& encoderl1 = * new EncoderL1Action();

   for( unsigned i = 0; i < _nServers; i++ )
   {
      _fsm.callback( TransitionId::Map,
                     new EncoderAllocAction( *cfg[i] ) );
   }
   _fsm.callback( TransitionId::L1Accept, &encoderl1 );
}
