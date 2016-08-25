#ifndef PDS_BLDSERVER
#define PDS_BLDSERVER

#include "EbServer.hh"
#include "BldSequenceSrv.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"
#include "pds/xtc/Datagram.hh"

namespace Pds {
  class BldServer : public EbServer, BldSequenceSrv
  {
  public:
    BldServer(const Ins& ins,
		const Src& src,
		unsigned   maxbuf);
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
  public:
    NetServer&      server();
    unsigned        fiducials() const;
  private:
    NetServer   _server;
    Src         _client;
  };
};


inline Pds::NetServer& Pds::BldServer::server() 
{
  return _server;
}

inline unsigned Pds::BldServer::fiducials() const
{
  return reinterpret_cast<const Pds::Datagram*>(_server.datagram())->seq.stamp().fiducials();
}

#endif
