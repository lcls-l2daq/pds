#ifndef Pds_TprDS_Server_hh
#define Pds_TprDS_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbSequenceSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pdsdata/xtc/L1AcceptEnv.hh"
#include "pdsdata/xtc/Sequence.hh"

namespace Pds {
  namespace Tpr { class RxDesc; }
  namespace TprDS {
    class Server : public EbServer,
                   public EbSequenceSrv
    {
    public:
      Server( int fd, const Src& );
      virtual ~Server();
    
      //  Eb interface
      void       dump ( int detail ) const {}
      bool       isValued( void ) const    { return true; }
      const Src& client( void ) const      { return _xtc.src; }

      //  EbSegment interface
      const Xtc& xtc( void ) const    { return _xtc; }
      unsigned   offset( void ) const { return 0; }
      unsigned   length() const       { return _xtc.extent; }

      //  Eb-key interface
      EbServerDeclare;

      virtual const Sequence&     sequence() const { return _seq; }
      virtual const L1AcceptEnv&  env     () const { return _env; }


      //  Server interface
      int pend( int flag = 0 ) { return -1; }
      int fetch( char* payload, int flags );
      bool more() const { return false; }

      unsigned count() const;

    private:
      int          _fd;
      Xtc          _xtc;
      Sequence     _seq;
      L1AcceptEnv  _env;
      uint32_t*    _buff;
      Tpr::RxDesc* _desc;
    };
  };
};

#endif
