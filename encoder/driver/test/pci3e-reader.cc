#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "pci3e.h"
#include "pci3e-wrapper.hh"

int main( int argc, char** argv )
{
   int ret;
   uint32_t was_read;
   unsigned int num_fifo_entries;
   unsigned int num_fifo_items;
   fd_set wait_fds;
   struct timeval tv;
   PCI3E::fifo_chan_entry fifo_chan_entry;
   uint8_t port_val;

   PCI3E::dev pci3e( "/dev/pci3e0" );
   PCI3E::channel chan0 = pci3e.get_channel( CHAN0 );

   // PCI3E::print_dbg = 1;

   printf( "Opening pci3e device...\n" );
   ret = pci3e.open();
   if( ret == -1 )
   {
      printf( "Error.  Exiting...\n" );
      return -1;
   }

   ret = pci3e.read_input_port( &port_val );

   // Read, update, and re-read encoder counter latch.
   ret = chan0.latch_counter( &was_read );

   printf( "Set falling edge (bit %d) on input 0 (bit %d) as a trigger src...\n",
           TRIG_CTRL_ENABLE0, TRIG_CTRL_INVERT0 );

   REG_TRIG_CTRL_t trig_ctrl;
   ret = pci3e.reg_read( REG_TRIG_CTRL, &trig_ctrl.whole );

   trig_ctrl.enable0 = TRIG_CTRL_ENABLE;
   trig_ctrl.invert0 = TRIG_CTRL_FALLING;

   ret = pci3e.reg_write( REG_TRIG_CTRL, trig_ctrl.whole );

   printf( "Clearing any previous trigger on input 0...\n" );
   ret = pci3e.clear_input_trigger( 0 );

   // Read various configuration items.
   ret = pci3e.reg_read( REG_TRIG_SETUP,      &was_read );
   ret = pci3e.reg_read( REG_QUAL_SETUP,      &was_read );
   ret = pci3e.reg_read( REG_NUM_SAMP,        &was_read );
   ret = pci3e.reg_read( REG_NUM_SAMP_REMAIN, &was_read );
   ret = pci3e.reg_read( REG_ACQ_CTRL,        &was_read );

   printf( "Enabling trigger to latch encoder 0...\n" );
   ret = chan0.enable_capture();

   printf( "Putting chan 0 in X1 mode...\n" );
   ret = chan0.set_quadrature_mode( QUAD_MODE_X1 );
   
   printf( "Setting counter mode to wraparound...\n" );
   ret = chan0.set_count_mode( COUNT_MODE_WRAP_FULL );

   printf( "Enabling channel 0...\n" );
   ret = chan0.enable();

   printf( "Enabling trigger to generate interrupt...\n" );
   pci3e.enable_interrupt_on_trigger();

   printf( "Enabling FIFO...\n" );
   ret = pci3e.enable_fifo();

   ret = pci3e.reg_read( REG_TS_COUNT, &was_read );
   printf( "Current TS = %u.\n", was_read );


   printf( "Clearing and stopping timestamp counter...\n" );
   pci3e.stop_timestamp_counter();

   ret = pci3e.reg_read( REG_TS_COUNT, &was_read );
   printf( "Current TS = %u.\n", was_read );

   printf( "Enabling timestamp counter...\n" );
   pci3e.start_timestamp_counter();
   ret = pci3e.reg_read( REG_TS_COUNT, &was_read );
   printf( "Current TS = %u.\n", was_read );

   ret = pci3e.reg_read( REG_FIFO_STAT_CTRL, &was_read );

   bool done = false;

   while( !done )
   {
      printf( "Waiting for external trigger - will quit on timeout...\n" );
      
      int high_fd = pci3e.get_fd();
      FD_ZERO( &wait_fds );
      FD_SET( high_fd, &wait_fds );
      tv.tv_sec = 5;
      tv.tv_usec = 0;

      ret = select( high_fd+1,  // Highest FD + 1
                    &wait_fds,  // fds to wait to become *readable*
                    NULL,       // fds to wait to become *writable*
                    NULL,       // fds that are in an exception state
                    &tv );      // duration to wait
      if( ret == -1 )
      {
         printf( "\tError!  errno=%d (%s).\n", errno, strerror( errno ) );
         done = true;
      }
      else if( !ret )
      {
         printf( "\tTimeout!  No readable data within timeout.\n" );
         done = true;
      }
      else
      {
         printf( "\tData ready: %d fds are readable.\n", ret );
         if( FD_ISSET( high_fd, &wait_fds ) )
         {
            ret = pci3e.get_num_fifo_items( &num_fifo_items );

            num_fifo_entries =
               PCI3E::fifo_num_items_to_entries( num_fifo_items );

            printf( "\tFIFO has %d items (%d entries)...\n",
                    num_fifo_items,
                    num_fifo_entries );

            printf( "\tReading FIFO...\n" );
            ret = chan0.read_fifo( &fifo_chan_entry );
            if( ret )
            {
               printf( "\tError!  ioctl=%d (%s).\n", ret, strerror(errno) );
            }
            else
            {
               printf( "\tEntry: ts=%u, ch0=0x%08x, flags=0x%08x.\n",
                       fifo_chan_entry.timestamp,
                       fifo_chan_entry.count,
                       fifo_chan_entry.flags );
            }
         }
         else
         {
            printf( "\tERROR: pci3e fd (%d) is not readable??\n",
                    pci3e.get_fd() );
            done = true;
         }
      }
   }

   pci3e.dump_regs();

   ret = pci3e.reg_read( REG_FIFO_STAT_CTRL, &was_read );

   pci3e.close();

   return 0;
}
