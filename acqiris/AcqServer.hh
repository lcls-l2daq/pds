#ifndef PDS_ACQSERVER
#define PDS_ACQSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"

#include "pds/xtc/Datagram.hh"

namespace Pds {

  class CDatagram;
  class DmaEngine;

  class AcqServer : public EbServer, public EbCountSrv {
  public:
    AcqServer(const Src& client, const TypeId& id);
    virtual ~AcqServer() {}
    
    enum Command {Header,Payload};
    int  payloadComplete();
    int  headerComplete(unsigned payloadSize,
			unsigned count,
			Damage dmg=Damage(0));
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    bool        more  () const;
    unsigned    offset() const;
    unsigned    length() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend  (int flag = 0);
    int      fetch (char* payload, int flags);
  public:
    unsigned count() const;
    void setDma(DmaEngine* dma);
  private:
    int        _pipefd[2];
    Xtc        _xtc;
    unsigned   _count;
    DmaEngine* _dma;
    Command    _cmd;
  };
}
#endif
