#ifndef PCI3E_DEV_HH
#define PCI3E_DEV_HH

#include <unistd.h>

#include "pdsdata/encoder/ConfigV1.hh"
#include "pdsdata/encoder/DataV1.hh"
#include "driver/pci3e-wrapper.hh"

namespace Pds
{
   class PCI3E_dev;
}

class Pds::PCI3E_dev
{
 public:
   PCI3E_dev( const char* pci3e_dev_node )
      : _pci3e( pci3e_dev_node ) {}
   ~PCI3E_dev() {}
    
   int open( void );
   // Clears the FIFO and leaves interrupts disabled.
   int configure( const Pds::Encoder::ConfigV1& config );

   // Clears FIFO and disables interrupts.
   int unconfigure();
   int get_data( Pds::Encoder::DataV1& data );
   int ignore_old_data( void );
   int get_fd()
      { return _pci3e.get_fd(); }

 private:
   PCI3E::dev _pci3e;
   PCI3E::channel _channel;
};
  
#endif
