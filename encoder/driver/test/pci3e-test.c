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
#include "pci3e-funcs.h"

void trig_handler( int signum )
{
   write( 0, "Got signal: ", 12 );
   write( 0, signum, sizeof(int) );
}


int latch_timestamp( int fd,
                     unsigned int* ptr_cmd_val,
                     unsigned int* ptr_ts_val )
{
   int ret;
   struct pci3e_io_struct ios;
   unsigned int cmd_val;

   // Do we need to read the command register?
   if( !ptr_cmd_val )
   {
      printf( "Calling ioctl to read Command register... " );

      ios.offset = 7 * 4;
      ios.value = 0;
      cmd_val = 0;

      ret = ioctl( fd, IOCREAD, &ios );
      if( ret )
      {
         printf( "Error!  ioctl=%d (%s).\n", ret, strerror(errno) );
         goto latch_timestamp_exit;
      }
      cmd_val = ios.value;

      printf( "Success!  CMD=0x%08x.\n", cmd_val );
   }

   // Latch only occurs on rising edge, so we may need to lower latch
   // bit first.

   if( cmd_val & (1 << CMD_TS_LATCH) )
   {
      printf( "Calling ioctl to lower TS latch... " );

      cmd_val &= ~(1 << CMD_TS_LATCH);
      ios.value = cmd_val;

      ret = ioctl( fd, IOCWRITE, &ios );
      if( ret )
      {
         printf( "Error!  ioctl=%d (%s).\n", ret, strerror(errno) );
         goto latch_timestamp_exit;
      }
   
      printf( "Success!\n" );
   }

   printf( "Calling ioctl to latch TS counter... " );

   cmd_val |= 1 << CMD_TS_LATCH;
   ios.value = cmd_val;

   ret = ioctl( fd, IOCWRITE, &ios );
   if( ret )
   {
      printf( "Error!  ioctl=%d (%s).\n", ret, strerror(errno) );
      goto latch_timestamp_exit;
   }
   
   printf( "Success!\n" );

   printf( "Calling ioctl to read TS latch register... " );

   ios.offset = 15 * 4;
   ios.value = 0;

   ret = ioctl( fd, IOCREAD, &ios );
   if( ret )
   {
      printf( "Error!  ioctl=%d (%s).\n", ret, strerror(errno) );
      goto latch_timestamp_exit;
   }
   *ptr_ts_val = ios.value;

   printf( "Success!  TS Latch=0x%08x.\n", *ptr_ts_val );

 latch_timestamp_exit:
   if( ptr_cmd_val )
   {
      *ptr_cmd_val = cmd_val;
   }
   return ret;
}


int main( int argc, char** argv )
{
   int fd;
   int ret;
   int flags;
   __sighandler_t prev_handler;
   struct pci3e_io_struct ios;
   unsigned int cmd_val;
   unsigned int latch_val;
   char ch;
   uint32_t was_read;
   unsigned int num_fifo_entries;
   fd_set wait_fds;
   struct timeval tv;
   unsigned int fifo_entries[MAX_FIFO_ENTRIES][FIFO_ENTRY_LEN];
   int i, j;

   printf( "Opening pci3e device...\n" );

   fd = open( "/dev/pci3e0", O_RDWR );
   printf( "open() = %d (errno=%d, '%s').\n",
           fd, errno, strerror(errno) );
   if( fd == -1 )
   {
      printf( "Error.  Exiting...\n" );
      return -1;
   }

   // printf( "Installing signal handler for SIGIO...\n" );
   // prev_handler = signal( SIGIO, &trig_handler );
   // if( prev_handler == SIG_ERR )
   // {
   //    printf( "ERROR!!" );
   //    close( fd );
   //    return -1;
   // }
   // printf( "Enabling asynchronous notification from PCI-3E...\n" );
   // fcntl( fd, F_SETOWN, getpid() );
   // flags = fcntl(fd, F_GETFL );
   // if( fcntl( fd, F_SETFL, flags | FASYNC ) == -1 ) {
   //    printf( "Error: fcntl() -1, errno=%d (%s).\n",
   //            errno, strerror(errno) );
   //    close(fd);
   //    return -1;
   // }

   printf( "Enabling non-blocking access to pci3e...\n" );
   flags = fcntl(fd, F_GETFL );
   if( fcntl( fd, F_SETFL, flags | O_NONBLOCK ) == -1 ) {
      printf( "Error: fcntl() -1, errno=%d (%s).\n",
              errno, strerror(errno) );
      close(fd);
      return -1;
   }

   ret = pci3e_reg_read( fd, REG_INP_PORT, &was_read );

   ret = pci3e_reg_read( fd,
                         GET_CHAN_REG( 0, REG_LATCH_OUTPUT ),
                         &was_read );

   ret = pci3e_reg_write( fd,
                          GET_CHAN_REG( 0, REG_LATCH_OUTPUT ),
                          0 );

   ret = pci3e_reg_read( fd,
                         GET_CHAN_REG( 0, REG_LATCH_OUTPUT ),
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_TRIG_CTRL,
                         &was_read );

   printf( "Set falling edge (bit %d) on input 0 (bit %d) as a trigger src...\n",
           TRIG_CTRL_ENABLE0, TRIG_CTRL_INVERT0 );
   ret = pci3e_reg_write( fd,
                          REG_TRIG_CTRL,
                            ( TRIG_CTRL_FALLING << TRIG_CTRL_INVERT0 )
                          | ( TRIG_CTRL_ENABLE  << TRIG_CTRL_ENABLE0 ) );

   printf( "Clearing any previous trigger on input 0...\n" );
   ret = pci3e_reg_write( fd,
                          REG_TRIG_STAT,
                          mask( TRIG_DET_IN0 ) );

   ret = pci3e_reg_read( fd,
                         REG_TRIG_SETUP,
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_QUAL_SETUP,
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_NUM_SAMP,
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_NUM_SAMP_REMAIN,
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_ACQ_CTRL,
                         &was_read );

   ret = pci3e_reg_read( fd,
                         REG_FIFO_STAT_CTRL,
                         &was_read );

   num_fifo_entries = FIFO_GET_DATA_CNT(was_read);

   if( num_fifo_entries )
   {
      printf( "Disabling FIFO...\n" );
      ret = pci3e_reg_write( fd,
                             REG_FIFO_STAT_CTRL,
                             FIFO_OFF << FIFO_ENABLE );

      printf( "Draining FIFO of %d entries...\n", num_fifo_entries );

      ios.offset = REG_TO_OFFSET( REG_FIFO_READ );
      for( i = 0; i < num_fifo_entries; ++i )
      {
         j = i % FIFO_ENTRY_LEN;
         if( ! j )
         {
            printf( "Read FIFO entry %d.\n", i / FIFO_ENTRY_LEN );
         }

         ret = pci3e_reg_write( fd,
                                REG_FIFO_READ,
                                JUNK );

         ret = pci3e_reg_read( fd,
                               REG_FIFO_READ,
                               &was_read );

         fifo_entries[i/FIFO_ENTRY_LEN][j] = ios.value;
      }

      for( i = 0; i < num_fifo_entries; i += FIFO_ENTRY_LEN )
      {
         j = i / FIFO_ENTRY_LEN;
         printf( "Entry #%d: ts=%d, ch0=%p, ch1=%p, ch2=%p, inp=%p.\n",
                 j,
                 fifo_entries[j][0],
                 fifo_entries[j][1],
                 fifo_entries[j][2],
                 fifo_entries[j][3],
                 fifo_entries[j][4] );
      }

      ret = pci3e_reg_read( fd,
                            REG_FIFO_STAT_CTRL,
                            &was_read );
   }

   ret = pci3e_reg_read( fd,
                         REG_FIFO_ENABLE,
                         &was_read );

   ret = pci3e_reg_write( fd,
                          REG_FIFO_STAT_CTRL,
                          FIFO_ON << FIFO_ENABLE );

   ret = pci3e_reg_read( fd,
                         GET_CHAN_REG( CHAN0, REG_CTRL ),
                         &was_read );

   printf( "Putting chan 0 in X1 and enabling...\n" );
   ret = pci3e_reg_write( fd,
                          GET_CHAN_REG( CHAN0, REG_CTRL ),
                            was_read
                          | ( QUAD_MODE_X1 << CTRL_QUAD_MODE )
                          | ( 1 << CTRL_ENABLE ) );

   ret = pci3e_reg_read( fd,
                         GET_CHAN_REG( CHAN0, REG_CTRL ),
                         &was_read );

   printf( "Enabling trigger to latch encoder 0...\n" );
   ret = pci3e_reg_write( fd,
                          GET_CHAN_REG( CHAN0, REG_CTRL ),
                          was_read | mask( CTRL_ENABLE_CAP ) );

   printf( "Enabling trigger to generate interrupt...\n" );
   ret = pci3e_reg_write( fd,
                          REG_INT_CTRL,
                          mask( INT_CTRL_TRIG ) );

   printf( "Preparing to select()...\n" );
   FD_ZERO( &wait_fds );
   FD_SET( fd, &wait_fds );
   tv.tv_sec = 5;
   tv.tv_usec = 0;

   ret = pci3e_reg_read( fd,
                         REG_TS_COUNT,
                         &was_read );
   printf( "Current TS = %u.\n", was_read );


   printf( "Clearing and stopping timestamp counter...\n" );
   ret = pci3e_reg_write( fd,
                          REG_CMD,
                          mask( CMD_TS_HALT ) );

   ret = pci3e_reg_read( fd,
                         REG_TS_COUNT,
                         &was_read );
   printf( "Current TS = %u.\n", was_read );


   printf( "Enabling timestamp counter...\n" );
   ret = pci3e_reg_write( fd,
                          REG_CMD,
                          0 << CMD_TS_HALT );

   // printf( "Do a read on fd=%d (should be non-blocking)...\n", fd );
   // was_read = 0;
   // ret = read( fd, &was_read, 4 );
   // printf( "\tread() returned %d, errno=%d (%s).\n", ret, errno,
   //         strerror(errno) );

   printf( "Waiting for up to %d secs in select()...\n", tv.tv_sec );
   ret = select( fd+1,       // Highest FD + 1
                 &wait_fds,  // fds to wait to become *readable*
                 NULL,       // fds to wait to become *writable*
                 NULL,       // fds that are in an exception state
                 &tv );      // duration to wait
   if( ret == -1 )
   {
      printf( "\tError!  errno=%d (%s).\n", errno, strerror( errno ) );
   }
   else if( !ret )
   {
      printf( "\tTimeout!  No readable data within timeout.\n" );
   }
   else
   {
      printf( "\tData ready: %d fds are readable.\n", ret );
      if( FD_ISSET( fd, &wait_fds ) )
      {
         printf( "\tpci3e fd (%d) is readable.\n", fd );
      }
      else
      {
         printf( "\tERROR: pci3e fd (%d) is not readable??\n", fd );
      }
   }

   ios.offset = REG_TO_OFFSET( REG_FIFO_STAT_CTRL );
   ios.value = 0;
   printf( "Read FIFO Status/Control register (#%d at offset %d).\n",
           REG_FIFO_STAT_CTRL, ios.offset );
   ret = ioctl( fd, IOCREAD, &ios );
   if( ret )
   {
      printf( "Error!  ioctl=%d (%s).\n", ret, strerror(errno) );
   }
   else
   {
      printf( "FIFO status=0x%08x.\n", ios.value );
   }

   printf( "close()ing fd=%d.\n", fd );
   close( fd );

   return 0;
}
