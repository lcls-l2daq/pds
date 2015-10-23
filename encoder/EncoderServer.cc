#include "EncoderServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pdsdata/psddl/encoder.ddl.h"
#include "pds/config/EncoderDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

Pds::EncoderServer::EncoderServer( const Src& client )
   : _xtc( _encoderDataType, client ),
     _occSend(NULL)
{
   _xtc.extent = sizeof(EncoderDataType) + sizeof(Xtc);
}

unsigned Pds::EncoderServer::configure(const EncoderConfigType& config)
{
  if (!_encoder->isOpen()) {
    // ERROR
    char msgBuf[80];
    snprintf(msgBuf, 80, "Encoder: Failed to open %s.\n", _encoder->get_dev_name());
    _occSend->userMessage(msgBuf);
    return (1);
  }

   _count = 0;
   return _encoder->configure(config);
}

unsigned Pds::EncoderServer::unconfigure(void)
{
  unsigned rv = 0;
  _count = 0;

  if (_encoder->isOpen()) {
   rv = _encoder->unconfigure();
  }
  
  return (rv);
}

int Pds::EncoderServer::fetch( char* payload, int flags )
{
   Pds::Encoder::DataV2 data;
   int ret;
  static bool firstCall = true;

  if (!_encoder->isOpen()) {
    if (firstCall) {
      fprintf(stderr, "PCI3E_dev error: device is not open...\n" );
      firstCall = false;
    }
    return (-1);
  }
   // Verify that there is indeed data to read.

   ret = _encoder->get_data( data );
   if( ret )
   {
      printf( "PCI3E_dev error: fetch data had problems...\n" );
      // register damage?
      return 0;
   }

   // Copy data, modifying extent as appropriate
   if (_xtc.damage.value()) {
     // In case of damage, modify extent 
     _xtc.extent = sizeof(Xtc);
     // copy xtc header to payload and you're done
     memcpy(payload, &_xtc, sizeof(Xtc) );
   }
   else {
     // calculate xtc extent
     _xtc.extent = sizeof(Xtc) + sizeof(EncoderDataType);
     // copy xtc header to payload
     memcpy( payload, &_xtc, sizeof(Xtc) );
     // copy data to payload
     memcpy ( payload + sizeof(Xtc), &data, sizeof(EncoderDataType) );

   }

   _count++;
   
   return (_xtc.extent);
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

void EncoderServer::setOccSend(EncoderOccurrence* occSend)
{
  _occSend = occSend;
}
