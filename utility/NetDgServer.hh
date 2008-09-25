#ifndef PDS_NETDGSERVER
#define PDS_NETDGSERVER

#include "EbServer.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"

namespace Pds {
class NetDgServer : public EbServer
  {
  public:
    NetDgServer(const Ins& ins,
		const Src& src,
		unsigned   nbufs);
   ~NetDgServer();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const InXtc&   xtc   () const;
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
  };
}


inline Pds::NetServer& Pds::NetDgServer::server() 
{
  return _server;
}

inline const Pds::Sequence& Pds::NetDgServer::sequence() const
{
  return *(Pds::Sequence*)_server.datagram();
}

#endif
