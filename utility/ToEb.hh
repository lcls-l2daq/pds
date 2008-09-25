#ifndef PDS_TOEB
#define PDS_TOEB

#include "EbServer.hh"
#include "EbEventKey.hh"

#include "pds/xtc/Datagram.hh"

namespace Pds {

  class CDatagram;
  class ZcpDatagram;

  class ToEb : public EbServer {
  public:
    ToEb(const Src& client);
    virtual ~ToEb() {}
    
    int  send(const CDatagram*  );
    int  send(const ZcpDatagram*);
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const InXtc&   xtc   () const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  public:
    const Sequence& sequence() const;
    unsigned count() const;
  private:
    int      _pipefd[2];
    Src      _client;
    Datagram _datagram;
  };
}
#endif
