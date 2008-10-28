#ifndef PDS_ACQSERVER
#define PDS_ACQSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbEventKey.hh"

#include "pds/xtc/Datagram.hh"

namespace Pds {

  class CDatagram;
  class ZcpDatagram;
  class DmaEngine;

  class AcqServer : public EbServer {
  public:
    AcqServer(const Src& client);
    virtual ~AcqServer() {}
    
    enum Command {Header,Payload};
    int  payloadComplete();
    int  headerComplete(Datagram& dg);
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const InXtc&   xtc   () const;
    bool  more() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend  (int flag = 0);
    int      fetch (char* payload, int flags);
    int      fetch (ZcpFragment& , int flags);
    unsigned offset() const;
    unsigned length() const;
  public:
    const Sequence& sequence() const;
    unsigned count() const;
    void setDma(DmaEngine* dma);
  private:
    int        _pipefd[2];
    Src        _client;
    Datagram   _datagram;
    DmaEngine* _dma;
    Command    _cmd;
  };
}
#endif
