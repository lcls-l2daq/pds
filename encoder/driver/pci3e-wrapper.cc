#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "pci3e-wrapper.hh"
#include "pci3e.h"

int PCI3E::print_dbg = 0;

#define DBG(fmt...) if (PCI3E::print_dbg > 0) { printf(fmt); }

// Note - this order must exactly follow the order of the registers in
// the memory map.
const char* PCI3E::reg_to_name[REG_LAST] = {
   "REG_PRESET0",
   "REG_LATCH_OUTPUT0",
   "REG_MATCH0",
   "REG_CTRL0",
   "REG_STAT0",
   "REG_COUNTER0",
   "REG_TRANSFER_PRESET0",
   "REG_CMD",
   "REG_PRESET1",
   "REG_LATCH_OUTPUT1",
   "REG_MATCH1",
   "REG_CTRL1",
   "REG_STAT1",
   "REG_COUNTER1",
   "REG_TRANSFER_PRESET1",
   "REG_TS_LATCH",
   "REG_PRESET2",
   "REG_LATCH_OUTPUT2",
   "REG_MATCH2",
   "REG_CTRL2",
   "REG_STAT2",
   "REG_COUNTER2",
   "REG_TRANSFER_PRESET2",
   "REG_TS_COUNT",
   "REG_RESERVED",
   "REG_RESERVED",
   "REG_RESERVED",
   "REG_TRIG_CTRL",
   "REG_TRIG_STAT",
   "REG_RESERVED",
   "REG_SAMPLE_RATE_MULT",
   "REG_SAMPLE_CNT",
   "REG_RESERVED",
   "REG_RESERVED",
   "REG_INT_CTRL",
   "REG_INT_STAT",
   "REG_RESERVED",
   "REG_FIFO_ENABLE",
   "REG_FIFO_STAT_CTRL",
   "REG_FIFO_READ",
   "REG_INP_PORT",
   "REG_TRIG_SETUP",
   "REG_QUAL_SETUP",
   "REG_NUM_SAMP",
   "REG_NUM_SAMP_REMAIN",
   "REG_ACQ_CTRL",
   "REG_OUT_PORT",
   "REG_OUT_PORT_SETUP"
};
int PCI3E::longest_reg_name_len = 20;

const char* PCI3E::count_mode_to_name[1 << CTRL_COUNT_MODE_LEN] = {
   "COUNT_MODE_WRAP_FULL",
   "COUNT_MODE_LIMIT",
   "COUNT_MODE_HALT",
   "COUNT_MODE_WRAP_PRESET"
};

const char* PCI3E::quad_mode_to_name[1 << CTRL_QUAD_MODE_LEN] = {
   "QUAD_MODE_CLOCK_DIR",
   "QUAD_MODE_X1",
   "QUAD_MODE_X2",
   "QUAD_MODE_X4"
};

PCI3E::fifo_entry::fifo_entry( uint32_t* fifo_items )
{
   timestamp = fifo_items[FIFO_ENTRY_TS];
   io_stat   = fifo_items[FIFO_ENTRY_IOSTAT];
   for( int c = 0; c < NUM_CHAN; ++c )
   {
      count[c] = FIFO_ENTRY_GET_COUNT(fifo_items[FIFO_ENTRY_CH0+c]);
      flags[c] = FIFO_ENTRY_GET_FLAGS(fifo_items[FIFO_ENTRY_CH0+c]);
   }
}

PCI3E::fifo_chan_entry::fifo_chan_entry( int chan, fifo_entry* entry )
{
   timestamp = entry->timestamp;
   count     = entry->count[chan];
   flags     = entry->flags[chan];
}

void* operator new( size_t size, PCI3E::fifo_entry* mem )
{
   return mem;
}

void* operator new( size_t size, PCI3E::fifo_chan_entry* mem )
{
   return mem;
}


PCI3E::dev::dev( const char* dev_name )
   : _dev_name( dev_name ),
     _fd( 0 )
{
   printf( "PCI3E::dev::dev().\n" );
}

int PCI3E::dev::open( void )
{
   printf( "Opening pci3e device...\n" );

   _fd = ::open( _dev_name, O_RDWR );
   printf( "open() = %d (errno=%d, '%s').\n",
           _fd, errno, strerror(errno) );
   if( _fd == -1 )
   {
      return -1;
   }

   return 0;
}

int PCI3E::dev::close( void )
{
   int ret;
   
   if( _fd > 0 )
   {
      printf( "Closing pci3e device...\n" );
      ret = ::close( _fd );
   }
   _fd = 0;

   return ret;
}

PCI3E::dev::~dev()
{
   printf( "PCI3E::dev::~dev().\n" );
   if( _fd )
   {
      printf( "PCI3E::dev::~dev(): Closing device fd=%d...\n", _fd );
      this->close();
   }
}

int PCI3E::dev::reg_read( int reg, uint32_t* val  )
{
   struct pci3e_io_struct ios;
   int ret;

   ios.offset = REG_TO_OFFSET( reg );
   ios.value = 0;

   DBG( "Read: fd=%d, reg=%d (%s), off=%lu.\n",
        _fd, reg, reg_to_name[reg], ios.offset );

   ret = ioctl( _fd, IOCREAD, &ios );
   if( ret )
   {
      DBG( "\tError!  ioctl=%d (%s).\n", ret, strerror(errno) );
   }
   else
   {
      *val = ios.value;
      DBG( "\tSuccess!  %s = 0x%08x.\n", reg_to_name[reg], *val );
   }

   return ret;
}

int PCI3E::dev::reg_write( int reg, uint32_t val  )
{
   struct pci3e_io_struct ios;
   int ret;

   ios.offset = REG_TO_OFFSET( reg );
   ios.value = val;

   DBG( "Write: fd=%d, reg=%d (%s), off=%lu, val=0x%08x.\n",
        _fd, reg, reg_to_name[reg], ios.offset, val );

   ret = ioctl( _fd, IOCWRITE, &ios );
   if( ret )
   {
      DBG( "\tError!  ioctl=%d (%s).\n", ret, strerror(errno) );
   }

   return ret;
}

int PCI3E::dev::get_num_fifo_items( unsigned int* num_items )
{
   int ret;
   REG_FIFO_STAT_CTRL_t reg;

   ret = reg_read( REG_FIFO_STAT_CTRL, &reg.whole );
   *num_items = reg.num_items;

   return ret;
}

int PCI3E::dev::disable_fifo( void )
{
   REG_FIFO_ENABLE_t reg;

   reg.whole = 0;
   reg.enable = FIFO_OFF;
   return reg_write( REG_FIFO_ENABLE, reg.whole );
}

int PCI3E::dev::enable_fifo( void )
{
   REG_FIFO_ENABLE_t reg;

   reg.whole = 0;
   reg.enable = FIFO_ON;
   return reg_write( REG_FIFO_ENABLE, reg.whole );
}

int PCI3E::dev::clear_fifo( void )
{
   int ret;
   REG_FIFO_STAT_CTRL_t reg;

   reg.whole = 0;

   printf( "PCI3E::dev::clear_fifo().\n" );

   ret = reg_read( REG_FIFO_STAT_CTRL, &reg.whole );
   if( !ret )
   {
      return ret;
   }

   reg.init = 1;
   ret = reg_write( REG_FIFO_STAT_CTRL, reg.whole );
   if( !ret )
   {
      return ret;
   }

   reg.init = 0;
   return reg_write( REG_FIFO_STAT_CTRL, reg.whole );
}

int PCI3E::dev::read_fifo( struct PCI3E::fifo_entry* entry )
{
   uint32_t fifo_items[FIFO_ENTRY_LEN];
   int ret;

   ret = ioctl( _fd, IOCREADFIFO, fifo_items );
   if( ret )
   {
      return ret;
   }

   new (entry) fifo_entry( fifo_items );

   return 0;
}

int PCI3E::dev::stop_timestamp_counter( void )
{
   REG_CMD_t reg;

   reg.whole = 0;
   reg.stop_timestamp = 1;

   return reg_write( REG_CMD, reg.whole );
}

int PCI3E::dev::start_timestamp_counter( void )
{
   REG_CMD_t reg;

   reg.whole = 0;
   reg.stop_timestamp = 0;

   return reg_write( REG_CMD, reg.whole );
}

int PCI3E::dev::read_input_port( uint8_t* inp )
{
   int ret;
   uint32_t rval;
   
   ret = reg_read( REG_INP_PORT, &rval );
   *inp = (uint8_t) rval;
   return ret;
}

int PCI3E::dev::enable_interrupt_on_trigger( void )
{
   REG_INT_CTRL_t ctrl;

   ctrl.whole   = 0;
   ctrl.on_trig = 1;
   return reg_write( REG_INT_CTRL, ctrl.whole );
}

int PCI3E::dev::clear_input_trigger( int input_num )
{
   return reg_write( REG_TRIG_STAT, mask( TRIG_DET_IN0 + input_num ) );
}

int PCI3E::dev::set_quadrature_mode( uint32_t mode )
{
   int ret;

   for( int c = 0; c < NUM_CHAN; ++c )
   {
      ret = channel( this, c ).set_quadrature_mode( mode );
   }

   return ret;
}

int PCI3E::dev::enable_channels( void )
{
   int ret;

   for( int c = 0; c < NUM_CHAN; ++c )
   {
      ret = channel( this, c ).enable();
   }

   return ret;
}

int PCI3E::dev::enable_capture( void )
{
   int ret;

   for( int c = 0; c < NUM_CHAN; ++c )
   {
      ret = channel( this, c ).enable_capture();
   }

   return ret;
}

int PCI3E::dev::dump_regs( void )
{
   struct rng_t { int first; int last; } rng[] = {
      {0, REG_TS_COUNT},
      {REG_TRIG_CTRL, REG_TRIG_STAT},
      {REG_SAMPLE_RATE_MULT, REG_SAMPLE_CNT},
      {REG_INT_CTRL, REG_INT_STAT },
      {REG_FIFO_ENABLE, REG_OUT_PORT_SETUP}
   };
   int num_ranges = 5;
   int i;
   int r;
   int prev_dbg = PCI3E::print_dbg;
   unsigned read;
   int ret;

   PCI3E::print_dbg = 0;

   for( i = 0; i < num_ranges; ++i )
   {
      for( r = rng[i].first; r <= rng[i].last; ++r )
      {
         ret = reg_read( r, &read );
         printf( "%-*s = 0x%08x\n",
                 longest_reg_name_len, reg_to_name[r], read );
      }
   }

   PCI3E::print_dbg = prev_dbg;

   return 0;
}

////////////////////////////////////////////////////////////////////////

int PCI3E::channel::reg_read(  int reg, uint32_t* val )
{
   return _dev->reg_read( GET_CHAN_REG(_chan, reg), val );
}

int PCI3E::channel::reg_write( int reg, uint32_t  val )
{
   return _dev->reg_write( GET_CHAN_REG(_chan, reg), val );
}

int PCI3E::channel::latch_counter( uint32_t*  count )
{
   int ret;

   ret = reg_read(  REG_LATCH_OUTPUT, count );
   ret = reg_write( REG_LATCH_OUTPUT, JUNK );
   return reg_read( REG_LATCH_OUTPUT, count );
}

int PCI3E::channel::read_fifo( fifo_chan_entry* chan_entry )
{
   fifo_entry entry;
   int ret = _dev->read_fifo( &entry );
   new (chan_entry) fifo_chan_entry( _chan, &entry );
   return ret;
}

int PCI3E::channel::set_quadrature_mode( uint32_t mode )
{
   int ret;
   REG_CTRL_t reg;

   ret = reg_read( REG_CTRL, &reg.whole );

   reg.quad_mode = mode;

   return reg_write( REG_CTRL, reg.whole );
}

int PCI3E::channel::set_count_mode( uint32_t mode )
{
   int ret;
   REG_CTRL_t reg;

   ret = reg_read( REG_CTRL, &reg.whole );

   reg.count_mode = mode;

   return reg_write( REG_CTRL, reg.whole );
}

int PCI3E::channel::enable( void )
{
   REG_CTRL_t reg;

   reg_read( REG_CTRL, &reg.whole );
   reg.enable = 1;
   return reg_write( REG_CTRL, reg.whole );
}

int PCI3E::channel::enable_capture( void )
{
   REG_CTRL_t reg;

   reg_read( REG_CTRL, &reg.whole );
   reg.enable_capture = 1;
   return reg_write( REG_CTRL, reg.whole );
}
