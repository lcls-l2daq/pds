#ifndef PDS_BLDSERVER
#define PDS_BLDSERVER

#include "EbServer.hh"
#include "EbSequenceSrv.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"
#include "pds/service/ZcpFragment.hh"

namespace Pds {
  class BldServer : public EbServer, public EbSequenceSrv
  {
  public:
    BldServer(const Ins& ins,
	      const Src& src,
	      unsigned   nbufs);
   ~BldServer();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&   xtc   () const;
    bool           more  () const;
    unsigned       length() const;
    unsigned       offset() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  public:
    NetServer&      server();
    const Sequence& sequence() const;
  private:
    NetServer   _server;
    Src         _client;
    Xtc       _xtc;
    ZcpFragment _dg;
  };
}


inline Pds::NetServer& Pds::BldServer::server() 
{
  return _server;
}

inline const Pds::Sequence& Pds::BldServer::sequence() const
{
  return *(Pds::Sequence*)_server.datagram();
}

#endif
