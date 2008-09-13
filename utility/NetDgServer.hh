#ifndef PDS_NETDGSERVER
#define PDS_NETDGSERVER

#include "EbServer.hh"
#include "pds/service/NetServer.hh"

namespace Pds {
class NetDgServer : public EbServer
  {
  public:
    NetDgServer(const Src& client,
		const Ins& ins,
		unsigned   nbufs);
   ~NetDgServer();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    unsigned    keyTypes()             const;
    const char* key     (EbKey::Type)  const;
    bool        matches (const EbKey&) const;
    bool        isValued()             const;
    //  EbSegment interface
    const InXtc&   xtc   () const;
    bool           more  () const;
    unsigned       length() const;
    unsigned       offset() const;
  public:
    //  Server interface
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  public:
    NetServer& server();
  private:
    NetServer   _server;
    EbPulseId   _pulseId;
  };
}


inline Pds::NetServer& Pds::NetDgServer::server() 
{
  return _server;
}

#endif
