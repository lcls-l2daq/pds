#include "EncoderServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pdsdata/encoder/DataV1.hh"
#include "pds/config/EncoderDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

Pds::EncoderServer::EncoderServer( const Src& client )
   : _xtc( _encoderDataType, client )
{
   _xtc.extent = sizeof(EncoderDataType) + sizeof(Xtc);
}

unsigned Pds::EncoderServer::configure(const EncoderConfigType& config)
{
   _count = 0;
   return _encoder->configure(config);
}

unsigned Pds::EncoderServer::unconfigure(void)
{
   _count = 0;
   return _encoder->unconfigure();
}

int Pds::EncoderServer::fetch( char* payload, int flags )
{
   Pds::Encoder::DataV1 data;
   int ret;

   // Verify that there is indeed data to read.

   ret = _encoder->get_data( data );
   if( ret )
   {
      printf( "PCI3E_dev error: fetch data had problems...\n" );
      // register damage?
      return 0;
   }

   // FIXME: What *is* this funky calculation??

   int xtc_size = sizeof(Xtc);
   int data_size = sizeof(EncoderDataType);
   memcpy( payload, &_xtc, xtc_size );
   memcpy( payload + xtc_size, &data, data_size );
   _count++;
   return xtc_size + data_size;
}

unsigned EncoderServer::count() const
{
   if( ( _count % 1000 ) == 0 )
   {
      printf( "in EncoderServer::count, fd %d: returning %d\n",
              fd(), _count-1 );
   }
   return _count - 1;
}

void EncoderServer::setEncoder( PCI3E_dev* pci3e )
{
   _encoder = pci3e;
   fd( _encoder->get_fd() );
   _encoder->unconfigure();
}
