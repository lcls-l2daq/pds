#ifndef PDS_EVRSERVER
#define PDS_EVRSERVER

#include "EbServer.hh"
#include "EbCountSrv.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/EvrDatagram.hh"

namespace Pds {
  class InletWire;
  class EvrServer : public EbServer, EbCountSrv
  {
  public:
    EvrServer(const Ins& ins,
	      const Src& src,
              InletWire& inlet,
	      unsigned   nbufs);
   ~EvrServer();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    bool        isRequired()           const;
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
    unsigned        count() const;
  private:
    NetServer     _server;
    Src           _client;
    InletWire&    _inlet;
    GenericPool   _occPool;
  };
}


inline Pds::NetServer& Pds::EvrServer::server() 
{
  return _server;
}

inline const Pds::Sequence& Pds::EvrServer::sequence() const
{
  const Pds::EvrDatagram& dg = reinterpret_cast<const Pds::EvrDatagram&>(*_server.datagram());
  return dg.seq;
}

inline unsigned Pds::EvrServer::count() const
{
  const Pds::EvrDatagram& dg = reinterpret_cast<const Pds::EvrDatagram&>(*_server.datagram());
  return dg.evr;
}

#endif
