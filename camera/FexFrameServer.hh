#ifndef Pds_FexFrameServer_hh
#define Pds_FexFrameServer_hh
//=============================================
//  class FexFrameServer
//
//  An instanciation of EbServer that provides
//  feature extracted camera frames for the event builder.
//=============================================

#include "pdsdata/xtc/Xtc.hh"
#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/camera/TwoDMoments.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/config/FrameFexConfigType.hh"

namespace Pds {

  class DmaSplice;
  class Frame;
  class TwoDMoments;

  class FexFrameServer : public EbServer, public EbCountSrv {
  public:
    FexFrameServer (const Src&,
		    DmaSplice&);
    ~FexFrameServer();
  public:
    void                            post(FrameServerMsg*);
    void                            setFexConfig(const FrameFexConfigType&);
    void                            setCameraOffset(unsigned);

    const FrameFexConfigType& Config();
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
    int      fetch       (ZcpFragment& , int flags);
  public:
    unsigned count() const;
  private:
    unsigned _post_fex  (void* xtc, const FrameServerMsg* msg) const;
    unsigned _post_frame(void* xtc, const FrameServerMsg* msg) const;

    TwoDMoments _feature_extract( const Frame&          frame,
				  const unsigned short* frame_data ) const;

    int _queue_frame( const Frame&    frame,
		      FrameServerMsg* msg,
		      ZcpFragment&    zfo );
    int _queue_fex  ( const TwoDMoments&,
		      FrameServerMsg* msg,
		      ZcpFragment&    zfo );
    int _queue_fex_and_frame( const TwoDMoments&,
			      const Frame&    frame,
			      FrameServerMsg* msg,
			      ZcpFragment&    zfo );
  private:    
    DmaSplice& _splice;
    bool       _more;
    unsigned   _offset;
    int        _fd[2];
    Xtc        _xtc;
    unsigned   _count;
    LinkedList<FrameServerMsg> _msg_queue;
    const FrameFexConfigType* _config;
    unsigned   _camera_offset;
    unsigned   _framefwd_count;
  };
}

#endif
