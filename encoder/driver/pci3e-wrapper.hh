#ifndef PCI3E_WRAPPER_HH
#define PCI3E_WRAPPER_HH

#include <stdio.h>
#include <stdint.h>
#include "pci3e.h"

namespace PCI3E
{
   struct fifo_entry;
   struct fifo_chan_entry;
   class channel;
   class dev;

   extern const char* reg_to_name[REG_LAST];
   extern int longest_reg_name_len;

   extern const char* count_mode_to_name[1 << CTRL_COUNT_MODE_LEN];
   extern const char* quad_mode_to_name[1 << CTRL_QUAD_MODE_LEN];

   extern int print_dbg;

   inline int fifo_num_items_to_entries( int num_items ) {
      return num_items / FIFO_ENTRY_LEN;
   }
}

struct PCI3E::fifo_entry
{
   fifo_entry() {}
   fifo_entry( uint32_t* fifo_items );

   uint32_t timestamp;
   uint32_t count[NUM_CHAN];
   uint8_t  flags[NUM_CHAN];
   uint8_t  io_stat;
};

struct PCI3E::fifo_chan_entry
{
   fifo_chan_entry() {}
   fifo_chan_entry( int chan, fifo_entry* entry );

   uint32_t timestamp;
   uint32_t count;
   uint8_t  flags;
};

class PCI3E::channel
{
 public:
   channel() : _dev( NULL ), _chan( -1 ) {}
   channel( PCI3E::dev* dev, int chan )
      : _dev( dev ), _chan( chan ) {}

   int reg_read(  int reg, uint32_t* val );

   int reg_write( int reg, uint32_t  val );

   int latch_counter( uint32_t* count );

   int read_fifo( fifo_chan_entry* chan_entry );

   int enable( void );
   int set_quadrature_mode( uint32_t mode );
   int set_count_mode( uint32_t mode );
   int enable_capture( void );

   PCI3E::dev* _dev;
   int _chan;
};

class PCI3E::dev
{
 public:
   dev() : _fd( 0 ) {}
   dev( const char* dev_name );
   ~dev();

   int open( void );
   int close( void );

   channel get_channel( int chan ) {
      return channel( this, chan );
   }
   int get_fd( void ) {
      return _fd;
   }

   int reg_read(  int reg, uint32_t* val );
   int reg_write( int reg, uint32_t  val );

   int get_num_fifo_items( unsigned int* num_items );
   int enable_fifo( void );
   int disable_fifo( void );
   int clear_fifo( void );
   int read_fifo( fifo_entry* entry );

   // The TS counter is cleared as a side effect of stopping it.
   int stop_timestamp_counter( void );
   int start_timestamp_counter( void );

   int read_input_port( uint8_t* inp );

   int enable_interrupt_on_trigger( void );
   int clear_input_trigger( int input_num );

   // These functions perform the same operation across all channels
   // in the device.
   int enable_channels( void );
   int set_quadrature_mode( uint32_t mode );
   int enable_capture( void );

   int dump_regs( void );

 private:
   const char* _dev_name;
   int _fd;
};

#endif
