#ifndef Pds_EvrFifoServer_hh
#define Pds_EvrFifoServer_hh
//=============================================
//  class EvrFifoServer
//
//  An instanciation of EbServer that provides
//  camera frames for the event builder.
//=============================================

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class EvrFifoServer : public EbServer, public EbCountSrv {
  public:
    EvrFifoServer (const Src&);
    ~EvrFifoServer();
  public:
    void        post(unsigned count, unsigned fiducial);
    void        clear();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    bool        more  () const;
    unsigned    length() const;
    unsigned    offset() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
  public:
    unsigned count() const;
  protected:
    bool       _more;
    unsigned   _offset;
    int        _fd[2];
    Xtc        _xtc;
    unsigned   _count;
  };
}

#endif
