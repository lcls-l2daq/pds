#ifndef PDS_IPIMBSERVER
#define PDS_IPIMBSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/IpimbConfigType.hh"

#include "pds/ipimb/IpimBoard.hh"


namespace Pds {

  class IpimbServer : public EbServer, public EbCountSrv {
  public:
    IpimbServer(const Src& client, const int baselineSubtraction, const int polarity);
    virtual ~IpimbServer() {}
    
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    unsigned    offset() const;
    unsigned    length() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend  (int flag = 0);
    int      fetch (char* payload, int flags);
    int      fetch (ZcpFragment& , int flags);
  public:
    unsigned count() const;
    void setIpimb(IpimBoard* ipimb, char* serialDevice);

  public:
    unsigned configure(IpimbConfigType& config);
    unsigned unconfigure();

  private:
    Xtc _xtc;
    unsigned _count;
    IpimBoard* _ipimBoard;
    char* _serialDevice;
    int _baselineSubtraction, _polarity;
  };
}
#endif
