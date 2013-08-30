#include "pci3e_dev.hh"
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pdsdata/psddl/encoder.ddl.h"
#include "driver/pci3e-wrapper.hh"
#include "driver/pci3e.h"

#define BAIL_ON_FAIL(what) {     \
   ret = what;                   \
   if( ret ) {                   \
      printf( "Fail: %s = %d\n", \
              #what, ret );      \
      return ret;                \
   }                             \
}

int Pds::PCI3E_dev::open( void )
{
   int ret;

  ret = _pci3e.open();
  if (!ret) {
    _isOpen = true;
    _fd = _pci3e.get_fd();
  } else {
    printf("Fail: _pci3e.open() = %d\n", ret);
    // fail gracefully: open /dev/null to get a valid fd
    _fd = ::open("/dev/null", O_RDWR);
  }
  return (ret);
}

int Pds::PCI3E_dev::configure( const Pds::Encoder::ConfigV2& config )
{
   int ret;
   uint32_t regval;
   
   BAIL_ON_FAIL( _pci3e.disable_fifo() );
   BAIL_ON_FAIL( _pci3e.clear_fifo() );

   regval  = config.count_mode() << CTRL_COUNT_MODE;
   regval |= config.quadrature_mode() << CTRL_QUAD_MODE;
   regval |= mask( CTRL_ENABLE );
   regval |= mask( CTRL_ENABLE_CAP );

   PCI3E::channel channel;
   for (unsigned chan=0;chan<NUM_CHAN;chan++) {
     channel = _pci3e.get_channel( chan );
     BAIL_ON_FAIL( channel.reg_write( REG_CTRL, regval ) );
     BAIL_ON_FAIL( channel.reg_write( REG_STAT, STAT_RESET_FLAGS ) );
   }

   BAIL_ON_FAIL( _pci3e.stop_timestamp_counter() );
   BAIL_ON_FAIL( _pci3e.start_timestamp_counter() );

   BAIL_ON_FAIL( _pci3e.reg_write( REG_INT_STAT, STAT_RESET_FLAGS ) );

   // Make an edge on the specified external input cause a trigger.
   // The logic is inverted because the PCI-3E uses active low inputs.
   regval  = TRIG_CTRL_ENABLE << config.input_num();
   regval |= ( config.input_rising() ? TRIG_CTRL_FALLING : TRIG_CTRL_RISING )
     << ( TRIG_CTRL_INVERT0 + config.input_num() );
   BAIL_ON_FAIL( _pci3e.reg_write( REG_TRIG_CTRL, regval ) );

   BAIL_ON_FAIL( _pci3e.reg_write( REG_TRIG_STAT, STAT_RESET_FLAGS ) );

   // Ensure that logging mode isn't enabled.
   BAIL_ON_FAIL( _pci3e.reg_write( REG_TRIG_SETUP, TRIG_QUAL_ALL_OFF ) );
   BAIL_ON_FAIL( _pci3e.reg_write( REG_QUAL_SETUP, TRIG_QUAL_ALL_OFF ) );
   BAIL_ON_FAIL( _pci3e.reg_write( REG_ACQ_CTRL,   ACQ_ALL_OFF ) );

   BAIL_ON_FAIL( _pci3e.enable_fifo() );
   BAIL_ON_FAIL( _pci3e.enable_interrupt_on_trigger() );

   _pci3e.dump_regs();

   return 0;
}

int Pds::PCI3E_dev::unconfigure( void )
{
   int ret;

   // Disable all external input triggers.
   BAIL_ON_FAIL( _pci3e.reg_write( REG_TRIG_CTRL, 0 ) );

   BAIL_ON_FAIL( _pci3e.disable_interrupt_on_trigger() );
   BAIL_ON_FAIL( _pci3e.disable_fifo() );
   BAIL_ON_FAIL( _pci3e.clear_fifo() );

   ret = _pci3e.enable_fifo();

   return 0;
}

int Pds::PCI3E_dev::get_data( Pds::Encoder::DataV2& data )
{
   int ret;
   unsigned int num_items;
   PCI3E::fifo_entry entry;

   BAIL_ON_FAIL( _pci3e.get_num_fifo_items( &num_items ) );
   if( PCI3E::fifo_num_items_to_entries( num_items ) == 0 )
   {
      return -1;
   }

   BAIL_ON_FAIL( _pci3e.read_fifo( &entry ) );

   *new(&data) Pds::Encoder::DataV2(entry.timestamp,
                                    entry.count);
   return 0;
}
