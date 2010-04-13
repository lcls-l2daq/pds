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

#define NS_PER_SEC (1000000000ULL)

// The fiducial clock runs at 360Hz.
#define FIDUCIAL_PER_SEC (360ULL)

// The encoder clock runs at 33MHz.  Came from lab measurements.
#define ENCODER_TICK_PER_SEC (33211432ULL)

#define NUM_ENCODER_TICKS (1ULL << 32)

// The encoder's timestamp clock runs at 33Mhz.
#define NS_PER_ENC_TICK ((double) NS_PER_SEC / curr_enc_tick_per_sec)

#define NS_PER_FIDUCIAL ((double) NS_PER_SEC / FIDUCIAL_PER_SEC)

#define ENCODER_TICK_OVERFLOW_NS (NUM_ENCODER_TICKS * NS_PER_ENC_TICK)

// This is the number of seconds it takes for the fiducial counter to
// overflow.
#define FIDUCIAL_OVERFLOW_SEC (Pds::TimeStamp::MaxFiducials / FIDUCIAL_PER_SEC)

// Same, but number of nanoseconds.
#define FIDUCIAL_OVERFLOW_NS (SEC_FIDUCIAL_WRAP * NS_PER_SEC)

// Maximum difference in nanoseconds that the EVR and encoder
// timestamps can differ to still be a valid event: 10us.
#define EVR_ENC_MAX_DIFF_NS (10000)

// A global variable (bad!).  This holds the current configuration.  I
// need this to "remember" a configuration item and use it in some
// timestamp calculations in the L1Accept transition.
static unsigned long long curr_enc_tick_per_sec;

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
   EncoderL1Action();

   InDatagram* fire(InDatagram* in);
   void validate( InDatagram* in );

   bool     _reset_on_next;
   uint32_t _last_fiducial;
   uint32_t _last_enc_timestamp;
   uint64_t _last_evr_timestamp_ns;
};

EncoderL1Action::EncoderL1Action()
   : _reset_on_next( true ),
     _last_fiducial( 0 ),
     _last_enc_timestamp( 0 ),
     _last_evr_timestamp_ns( 0 )
{}

InDatagram* EncoderL1Action::fire(InDatagram* in)
{
   Datagram& dg = in->datagram();

   if (int d = dg.xtc.damage.value())
   {
      printf("Damage 0x%x\n",d);
   }
   else
   {
      validate( in );
   }

   return in;
}

// Determine if the current encoder measurement corresponds to the
// current evr trigger by comparing timestamps with the previous
// measurement.  There are three sources of timestamps:
//
// 1. From evr: fiducial count.
// 2. From evr: clock time.
// 3. From encoder: 33MHz counter.
//
// Our goal is to ensure that the time difference in encoder
// timestamps is equivalent to the time difference in evr timestamps.
// Once we've detected an error, we increase the damage on the event.
// We also "wipe out" the stored (previous) timestamps and start all
// over again - if we didn't do this, *every* event after the first
// would cause damage.
//
// NOTE: We cannot definitively assert that the encoder timestamp is
// correct because we have no absolute reference for it.  The encoder
// timestamp itself wraps every 130.15 seconds (32-bit value that
// counts at 33MHz).  Thus, if the timestamp wraps around, we have no
// way of knowing *how* many wraparounds occurred.  We could
// potentially be off by a multiple of 130.15 seconds.
//
// In this case, we err on the side of optimism - we assume the
// encoder timestamp is valid.
void EncoderL1Action::validate( InDatagram* in )
{
   Encoder::DataV1* data;

   uint32_t curr_fiducial;
   uint32_t diff_fiducial;
   uint64_t diff_fiducial_ns;

   uint32_t curr_enc_timestamp;
   uint32_t diff_enc_timestamp;
   uint64_t diff_enc_timestamp_ns;

   uint64_t curr_evr_timestamp_ns;
   uint64_t diff_evr_timestamp_ns;

   uint64_t diff_evr_vs_enc_ns;

   Datagram& dg = in->datagram();
   data = (Encoder::DataV1*) ( dg.xtc.payload() + sizeof(Xtc) );

   curr_fiducial = dg.seq.stamp().fiducials();
   curr_enc_timestamp = data->_33mhz_timestamp;

   curr_evr_timestamp_ns =  dg.seq.clock().seconds() * NS_PER_SEC
                          + dg.seq.clock().nanoseconds();
   diff_evr_timestamp_ns =  curr_evr_timestamp_ns
                          - _last_evr_timestamp_ns;

   // Ensure we have enough information to determine validity.  If we
   // have no prior event, *or* if the encoder tick counter has
   // overflowed, then we have no way of determining correctness.
   if(    _reset_on_next
       || diff_evr_timestamp_ns > ENCODER_TICK_OVERFLOW_NS )
   {  // Just squirrel away information for this event and bail.
      _reset_on_next = false;
      _last_fiducial = curr_fiducial;
      _last_enc_timestamp = curr_enc_timestamp;
      _last_evr_timestamp_ns = curr_evr_timestamp_ns;
      // printf( "L1Action::validate(): First event - OK.\n" );
      // printf( "Enc Timestamp: %u.\n", _last_enc_timestamp );
      return;
   }

   // Since the encoder tick counter holds much less time than the
   // fiducial counter (130s vs. 300s), we don't need to check a
   // fiducial overflow.

   // Account for encoder wrap.
   if( curr_enc_timestamp > _last_enc_timestamp ) {
      diff_enc_timestamp = curr_enc_timestamp - _last_enc_timestamp;
   } else {
      diff_enc_timestamp =  NUM_ENCODER_TICKS
                          + curr_enc_timestamp - _last_enc_timestamp;
   }
   diff_enc_timestamp_ns = (uint64_t) ( diff_enc_timestamp * NS_PER_ENC_TICK );

   // Account for fiducial wrap.
   if( curr_fiducial > _last_fiducial ) {
      diff_fiducial = curr_fiducial - _last_fiducial;
   } else {
      diff_fiducial =  Pds::TimeStamp::MaxFiducials
                     + curr_fiducial - _last_fiducial;
   }
   diff_fiducial_ns = (uint64_t) ( diff_fiducial * NS_PER_FIDUCIAL );

   if( diff_enc_timestamp_ns > diff_fiducial_ns ) {
      diff_evr_vs_enc_ns = diff_enc_timestamp_ns - diff_fiducial_ns;
   } else {
      diff_evr_vs_enc_ns = diff_fiducial_ns - diff_enc_timestamp_ns;
   }

   // printf( "diff_evr_vs_enc_ns = %llu.\n", diff_evr_vs_enc_ns );

   if( diff_evr_vs_enc_ns > (uint64_t) EVR_ENC_MAX_DIFF_NS )
   {  // Failed - we must have missed an event.  In this case, we have
      // to assume that the next event is OK, or we'll *never* catch
      // up.  We have to assume that *everything* is bad at this
      // point.  Oops!  Just start over.
      _reset_on_next = true;
      dg.xtc.damage.increase( Pds::Damage::OutOfOrder );
      printf( "L1Action::validate(): Fail: diff=%llu, .\n",
              diff_evr_vs_enc_ns );
      printf( "\tcurr_fid=%u, curr_enc=%u, curr_evr=%llu.\n",
              curr_fiducial, curr_enc_timestamp, curr_evr_timestamp_ns );
      printf( "\tlast_fid=%u, last_enc=%u, last_evr=%llu.\n",
              _last_fiducial, _last_enc_timestamp, _last_evr_timestamp_ns );
      printf( "\tdiff_fid=%u, diff_enc=%u, diff_enc_ns=%llu.\n",
              diff_fiducial, diff_enc_timestamp, diff_enc_timestamp_ns );
      printf( "\tdiff_fid_ns=%llu, diff_evr_vs_enc_ns=%llu.\n",
              diff_fiducial_ns, diff_evr_vs_enc_ns );
   }
   else
   {
      _last_fiducial = curr_fiducial;
      _last_enc_timestamp = curr_enc_timestamp;
      _last_evr_timestamp_ns = curr_evr_timestamp_ns;
      // printf( "Enc Timestamp: %u.\n", _last_enc_timestamp );
   }
}

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
                                        1, // trigger on rising edge
                                        33211600 );
      _config.dump();

      _nerror = 0;
      for (unsigned i = 0; i < _nServers; i++)
      {
         // int len = *_cfg[i].fetch(*tr,_ipimbConfigType, &_config, sizeof(_config));
         _nerror += _server[i]->configure( _config );
      }
      curr_enc_tick_per_sec = _config._ticks_per_sec;

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
   int ret;

   printf("initialized with %d servers\n", _nServers);

   for (unsigned i = 0; i < _nServers; i++)
   {
      sprintf( portName, "/dev/pci3e%d", i );
      PCI3E_dev* encoder = new PCI3E_dev( portName );
      ret = encoder->open();
      // What to do if the open fails?
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
