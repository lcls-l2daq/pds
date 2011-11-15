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
#include "pds/config/FrameFexConfigType.hh"
#include "pds/mon/THist.hh"

namespace Pds {

  class Frame;
  class TwoDMoments;

  class FexFrameServer : public FrameServer {
  public:
    FexFrameServer (const Src&);
    ~FexFrameServer();
  public:
    void                            setFexConfig(const FrameFexConfigType&);
    void                            setCameraOffset(unsigned);

    const FrameFexConfigType& Config();
  public:
    //  Server interface
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  private:
    unsigned _post_fex  (void* xtc, const FrameServerMsg* msg) const;
    unsigned _post_frame(void* xtc, const FrameServerMsg* msg) const;

    TwoDMoments _feature_extract( const Frame&          frame,
				  const unsigned short* frame_data ) const;

  private:    
    const FrameFexConfigType* _config;
    unsigned   _camera_offset;
    unsigned   _framefwd_count;

    timespec        _tinput;
    THist           _hinput;
    THist           _hfetch;
  };
}

#endif
