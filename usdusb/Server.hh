#ifndef Pds_UsdUsb_Server_hh
#define Pds_UsdUsb_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/UsdUsbConfigType.hh"

namespace Pds {
  namespace UsdUsb {
    class Server : public EbServer,
		   public EbCountSrv {

    public:
      Server( const Src& client );
      virtual ~Server() {}
      
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
      void resetCount();

      void post(char*, unsigned);

    private:
      Xtc _xtc;
      unsigned _count;
      int _pfd[2];
    };
  };
};

#endif
