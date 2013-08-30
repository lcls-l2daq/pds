#ifndef PCI3E_DEV_HH
#define PCI3E_DEV_HH

#include <unistd.h>

#include "pdsdata/psddl/encoder.ddl.h"
#include "driver/pci3e-wrapper.hh"

namespace Pds
{
   class PCI3E_dev;
}

class Pds::PCI3E_dev
{
 public:
   PCI3E_dev( const char* pci3e_dev_node )
      : _pci3e( pci3e_dev_node ),
        _fd( 0 ),
        _isOpen( false ) {}

   ~PCI3E_dev() {}
    
   int open( void );
   // Clears the FIFO and leaves interrupts disabled.
   int configure( const Pds::Encoder::ConfigV2& config );

   // Clears FIFO and disables interrupts.
   int unconfigure();
   int get_data( Pds::Encoder::DataV2& data );
   int ignore_old_data( void );
   int get_fd()
      { return _fd; }
   int isOpen()
      { return _isOpen; }
   const char * get_dev_name()
      { return _pci3e.get_dev_name(); }

 private:
   PCI3E::dev _pci3e;
    int _fd;
    bool _isOpen;
};
  
#endif
