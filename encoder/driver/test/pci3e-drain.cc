#include <stdio.h>
#include <stdint.h>
#include "pci3e.h"
#include "pci3e-wrapper.hh"

int main( int argc, char** argv )
{
   int ret;
   uint32_t was_read;
   unsigned int num_fifo_items;
   unsigned int num_fifo_entries;
   int i, j;

   PCI3E::fifo_chan_entry fifo_entry;
   PCI3E::dev pci3e( "/dev/pci3e0" );
   PCI3E::channel chan0( &pci3e, 0 );

   PCI3E::print_dbg = 1;

   printf( "Opening pci3e device...\n" );
   ret = pci3e.open();
   if( ret == -1 )
   {
      printf( "Error.  Exiting...\n" );
      return -1;
   }

   ret = pci3e.get_num_fifo_items( &num_fifo_items );
   num_fifo_entries = PCI3E::fifo_num_items_to_entries( num_fifo_items );

   printf( "FIFO has %d data items.\n", num_fifo_entries );

   if( num_fifo_items )
   {
      printf( "Disabling FIFO...\n" );
      ret = pci3e.disable_fifo();

      printf( "Draining FIFO of %d items...\n", num_fifo_items );

      for( i = 0; i < num_fifo_entries; ++i )
      {
         ret = chan0.read_fifo( &fifo_entry );
         printf( "Entry #%u: timestamp=%u, count=0x%08x, flags=0x%02x.\n",
                 i,
                 fifo_entry.timestamp,
                 fifo_entry.count,
                 fifo_entry.flags );
      }

      int extra_items = num_fifo_items % FIFO_ENTRY_LEN;
      if( extra_items )
      {
         printf( "Draining %d extra items from incomplete FIFO write...\n",
                 extra_items );

         for( i = 0; i < extra_items; ++i )
         {
            ret = pci3e.reg_write( REG_FIFO_READ,
                                   JUNK );

            ret = pci3e.reg_read( REG_FIFO_READ,
                                  &was_read );
         }
      }

      REG_FIFO_STAT_CTRL_t stat;
      ret = pci3e.reg_read( REG_FIFO_STAT_CTRL,
                            &stat.whole );

      if( stat.is_empty )
      {
         printf( "FIFO is now empty.\n" );
      }
      else
      {
         printf( "ERROR!!  FIFO isn't empty!  Still has %d items!\n",
                 stat.num_items );
      }

      printf( "Re-enabling FIFO...\n" );
      ret = pci3e.enable_fifo();
   }

   pci3e.close();

   return 0;
}
