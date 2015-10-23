#ifndef PDS_NETDGSERVER
#define PDS_NETDGSERVER

#include "EbServer.hh"
#include "EbSequenceSrv.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"
#include "pds/xtc/Datagram.hh"

namespace Pds {
  class NetDgServer : public EbServer, EbSequenceSrv
  {
  public:
    NetDgServer(const Ins& ins,
		const Src& src,
		unsigned   maxbuf);
   ~NetDgServer();
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
  public:
    NetServer&      server();
    const Sequence& sequence() const;
    const Env&      env     () const;
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
  return reinterpret_cast<const Pds::Datagram*>(_server.datagram())->seq;
}

inline const Pds::Env&      Pds::NetDgServer::env     () const
{
  return reinterpret_cast<const Pds::Datagram*>(_server.datagram())->env;
}

#endif
