//! PicPortCL.hh
//! Camera class child that uses the Daisy/SeqDRAL Leutron API to
//! control a Camera connected to a PicPortCL frame grabber using
//! the Camera Link protocol.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#ifndef PDS_CAMERA_PICPORTCL
#define PDS_CAMERA_PICPORTCL

#include <grabber.h>
#include <daseq32.h>
#include <daar.h>

#include "pds/camera/CameraDriver.hh"
//#include "pds/camera/LvCamera.hh"
#include "pds/camera/FrameHandle.hh"
#include "pds/mon/THist.hh"

//#define PICPORTCL_NAME    "PicPortX CL Mono"
//#define PICPORTCL_NAME    "PicPortExpress CL Mono"
//#define PICPORTCL_NAME    "PicPortExpress CL Stereo"
//#define PICPORTCL_NAME    "PicPortX CL Mono PMC"

#define PICPORTCL_BASE_CONNECTOR    "CamLink Base Port 0 (HVPSync In 0)"
#define PICPORTCL_MEDIUM_CONNECTOR  "CamLink Medium Port 0"

namespace Pds {
  class FrameServerMsg;
  class GenericPool;
  class MonEntryTH1F;
};

namespace PdsLeutron {

  extern const int num_supported_grabbers;
  extern char* supported_grabbers[];

  class FrameHandle;
  class MyQueue;

  class PicPortCL: public Pds::CameraDriver {
  public:
    enum Mode {
      MODE_CONTINUOUS,
      MODE_EXTTRIGGER,
      MODE_EXTTRIGGER_SHUTTER,
    };
    enum State {
      CAMERA_UNCONFIGURED,
      CAMERA_READY,
      CAMERA_RUNNING,
      CAMERA_STOPPED,
    };
    enum NotifyType {
      NOTIFYTYPE_NONE,    // GetFrameHandle will return NULL if no frame is ready.
      NOTIFYTYPE_WAIT,    // GetFrame pause if no frame is ready.
      NOTIFYTYPE_SIGNAL,  // A signal will be generated when a new frame is ready.
      NOTIFYTYPE_POLL,    // Instead of a signal, uses a file descriptor poll method.
    };
    struct Status {
      char *CameraId;
      char *CameraName;
      unsigned long CapturedFrames;
      unsigned long DroppedFrames;
      enum State State;
    };
  public:
    PicPortCL(Pds::CameraBase& camera,
	      int grabberid = 0, 
              const char *grabberName = (const char *)NULL);
    virtual ~PicPortCL();
  public:  // Camera interface
    int initialize(Pds::UserMessage*);
    int start_acquisition(Pds::FrameServer&,  // post frames here
                          Pds::Appliance  &,  // post occurrences here
			  Pds::UserMessage*);
    int stop_acquisition();
  public:
    void handle();
  public:
    int SetNotification(enum NotifyType mode);
    int Init(Pds::UserMessage*);
    int Start();
    int Stop();
    int GetStatus(Status&);
  public:
    FrameHandle *GetFrameHandle();
    int SendCommand(char *szCommand, 
		    char *pszResponse, 
		    int iResponseBufferSize);
    int SendBinary(char *szBinary, int iBinarySize, char *pszResponse, int iResponseBufferSize);
    //    unsigned char* frameBufferBaseAddress() const;
    //    unsigned char* frameBufferEndAddress () const;

    void dump();
  private:
    static void ReleaseFrame(void *obj, FrameHandle *pFrame, void *arg);
  private:
    Pds::FrameServer*    _fsrv;
    Pds::Appliance*      _app;
    Pds::GenericPool*    _occPool;
    bool                 _outOfOrder;
    int                  _sig;
    unsigned             _nposts;
    timespec             _tsignal;
    Pds::THist           _hsignal;
  private:
    DaSeq32Cfg             _seqDralConfig;
    LvROI                  _roi;
    int                    _grabberId;
    const char *           _grabberName;
    const char *           GetConnectorName();
    //    DsyApp_Seq_AsyncReset* _pSeqDral;
    DsyApp_Seq32*          _pSeqDral;
    //    LvCamera*              _lvdev;
    MyQueue*               _queue;
  private:
    enum NotifyType _notifyMode;
    int             _notifySignal;
  protected:
    Status status;
  private:
    unsigned char*      _frameBufferBase;
    FrameHandle::Format _frameFormat;
  private:
    Pds::MonEntryTH1F* _depth;
    Pds::MonEntryTH1F* _interval;
    Pds::MonEntryTH1F* _interval_w;
    unsigned           _sample_sec;
    unsigned           _sample_nsec;
  };

}

#endif	// #ifndef PDS_CAMERA_PICPORTCL
