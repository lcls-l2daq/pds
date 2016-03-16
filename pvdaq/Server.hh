#ifndef Pds_PvDaq_Server_hh
#define Pds_PvDaq_Server_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/BldSequenceSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/client/Action.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Dgram.hh"

//
//  Sub classes will implement Action, fill(), and construct _xtc.
//
namespace Pds {
  namespace PvDaq {
    class Server : public EbServer,
		   public BldSequenceSrv,
                   public Action {

    public:
      Server();
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
      int fetch( char* payload, int flags );

      const Sequence& sequence() const { return _seq; }
      const Env&      env     () const { return _env; }

    protected:
      //  Subclasses call post when their data is ready
      void post(const void*);
      //  This routine will be called back to fill the data into the XTC
      virtual int  fill(char*,const void*) = 0;

    public:
      static Server* lookup(const char*, const Pds::DetInfo&);

    protected:
      Xtc        _xtc;
      int        _pfd[2];
      Sequence   _seq;
      Env        _env;
    };
  };
};

#endif
