#ifndef Pds_FccdFrameServer_hh
#define Pds_FccdFrameServer_hh
//=============================================
//  class FccdFrameServer
//
//  An instanciation of EbServer that provides
//  FCCD camera frames for the event builder.
//=============================================

#include "pds/camera/FrameServer.hh"
#include "pds/camera/TwoDMoments.hh"
#include "pds/config/FrameFccdConfigType.hh"

// reorder buffer must accommodate Row_Pixels * Column_Pixels shorts
#define FCCD_REORDER_BUFSIZE  (500 * 576 * sizeof(uint16_t))

namespace Pds {

  class Frame;
  class TwoDMoments;

  class FccdFrameServer : public FrameServer {
  public:
    FccdFrameServer (const Src&);
    ~FccdFrameServer();
  public:
    void                            setFccdConfig(const FrameFccdConfigType&);
    void                            setCameraOffset(unsigned);

    const FrameFccdConfigType& Config();
  public:
    //  Server interface
    int      fetch       (char* payload, int flags);
  private:
    unsigned _post_fccd  (void* xtc, const FrameServerMsg* msg) const;
    unsigned _post_frame(void* xtc, const FrameServerMsg* msg) const;
    int      _reorder_frame(FrameServerMsg* fmsg);


    TwoDMoments _feature_extract( const Frame&          frame,
				  const unsigned short* frame_data ) const;

  private:    
    const FrameFccdConfigType* _config;
    unsigned   _camera_offset;
    unsigned   _framefwd_count;
    char *     _reorder_tmp;
  };
}

#endif
