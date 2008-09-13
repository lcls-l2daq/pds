#ifndef PDS_BLDSERVER
#define PDS_BLDSERVER

#include "EbServer.hh"
#include "pds/service/NetServer.hh"
#include "pds/service/ZcpFragment.hh"

namespace Pds {
class BldServer : public EbServer
  {
  public:
    BldServer(const Src& client,
	      const Ins& ins,
	      unsigned   nbufs);
   ~BldServer();
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
    InXtc       _xtc;
    ZcpFragment _dg;
  };
}


inline Pds::NetServer& Pds::BldServer::server() 
{
  return _server;
}

#endif
