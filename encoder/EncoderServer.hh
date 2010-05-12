#ifndef PDS_ENCODERSERVER
#define PDS_ENCODERSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/EncoderConfigType.hh"

#include "pds/encoder/pci3e_dev.hh"


namespace Pds
{
   class EncoderServer;
}

class Pds::EncoderServer
   : public EbServer,
     public EbCountSrv
{
 public:
   EncoderServer( const Src& client );
   virtual ~EncoderServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtc.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtc; }
   unsigned   offset( void ) const { return sizeof(Xtc); }
   unsigned   length() const       { return _xtc.extent; }

   //  Eb-key interface
   EbServerDeclare;

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( ZcpFragment& , int flags ) { return 0; }
   int fetch( char* payload, int flags );

   unsigned count() const;
   void setEncoder( PCI3E_dev* pci3e );
   void withdraw();
   void reconnect();

   void setFakeCount( unsigned fakeCount )
      { _fakeCount = fakeCount; }
   unsigned configure(const EncoderConfigType& config);
   unsigned unconfigure(void);

 private:
   Xtc _xtc;
   unsigned _count;
   unsigned _fakeCount;
   PCI3E_dev* _encoder;
};

#endif
