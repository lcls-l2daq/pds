#ifndef TagServer_hh
#define TagServer_hh

#include "EbServer.hh"
#include "EbSequenceSrv.hh"
#include "EbEventKey.hh"
#include "pds/service/NetServer.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  class Inlet;
  class TagServer : public EbServer, EbSequenceSrv {
  public:
    TagServer(const Ins& ins,
              const Src& src,
              Inlet&     inlet,
              unsigned   nbufs);
    ~TagServer();
  public:
    const Src&     client()             const;
    //  EbSegment interface
    const Xtc&     xtc   () const;
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
    const Sequence& sequence() const;
    const Env&      env     () const;
  public:
    bool isValued  () const;
    bool isRequired() const;
  public:
    const char*    payload() const { return _payload; }
    NetServer&     server() { return _server; }
    void           dump  (int) const;
  private:
    NetServer     _server;
    Src           _client;
    Inlet&        _inlet;
    GenericPool   _occPool;
    char*         _payload;
  };
};

#endif
