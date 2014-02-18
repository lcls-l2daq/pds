#ifndef Pds_FexFrameServer_hh
#define Pds_FexFrameServer_hh
//=============================================
//  class FexFrameServer
//
//  An instanciation of EbServer that provides
//  feature extracted camera frames for the event builder.
//=============================================

#include "pds/camera/FrameServer.hh"
#include "pds/camera/TwoDMoments.hh"
#include "pds/mon/THist.hh"

namespace Pds {

  class CfgCache;
  class Frame;
  class InDatagram;
  class Transition;
  class TwoDMoments;
  class UserMessage;
  class GenericPool;

  class FexFrameServer : public FrameServer {
  public:
    FexFrameServer (const Src&);
    ~FexFrameServer();
  public:
    void                            allocate       (Transition*);
    bool                            doConfigure    (Transition*);
    void                            nextConfigure  (Transition*);
    InDatagram*                     recordConfigure(InDatagram*);
    UserMessage*                    validate       (unsigned,unsigned);
  public:
    //  Server interface
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  private:
    unsigned _post_fex  (void* xtc, const FrameServerMsg* msg) const;
    unsigned _post_frame(void* xtc, const FrameServerMsg* msg) const;

    TwoDMoments _feature_extract( const FrameServerMsg* msg) const;

  private:    
    CfgCache*    _config;
    GenericPool* _occPool;

    unsigned   _framefwd_count;

    timespec        _tinput;
    THist           _hinput;
    THist           _hfetch;
  };
}

#endif
