#ifndef Pds_FrameServer_hh
#define Pds_FrameServer_hh
//=============================================
//  class FrameServer
//
//  An instanciation of EbServer that provides
//  camera frames for the event builder.
//=============================================

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class FrameServerMsg;
  
  class FrameServer : public EbServer, public EbCountSrv {
  public:
    FrameServer (const Src&);
    ~FrameServer();
  public:
    void        post(FrameServerMsg*);
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
    //    int      fetch       (char* payload, int flags);  // subclasses implement this
    //    int      fetch       (ZcpFragment& , int flags);  // subclasses implement this
  public:
    unsigned count() const;
  protected:
    bool       _more;
    unsigned   _offset;
    int        _fd[2];
    Xtc        _xtc;
    unsigned   _count;
    LinkedList<FrameServerMsg> _msg_queue;
  };
}

#endif
