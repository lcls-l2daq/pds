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
#include "Camera.hh"

#define PICPORTCL_NAME    "PicPortX CL Mono"

namespace Pds {

class PicPortCL: public Camera {
  public:
    struct PicPortCLConfig {
      // Camera Link Serial Port configuration
      int Baudrate;
      unsigned long Parity;
      unsigned long ByteSize;
      unsigned long StopSize;
      char eotWrite;
      char eotRead;
      char sof;
      char eof;
      unsigned long Timeout_ms;
      // The PicPort HW does not have an accurate way of tracking captured
      // and dropped frames. We can deduct those numbers up to a certain
      // accuracy by looking at GetLastAcquiredImage() but if too many
      // frames are dropped we won't see it.
      // That's why a camera can opt to manage the buffers itself by setting
      // PicPortCLConfig.bUsePicportCounters to false.
      bool bUsePicportCounters;
    };

    PicPortCL(int _grabberid = 0);
    virtual ~PicPortCL();
    int SetNotification(enum NotifyType mode);
    int Init();
    int Start();
    int Stop();
    Frame *GetFrame();
    int SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize);
  protected:
    int DsyToErrno(LVSTATUS DsyError);
    struct PicPortCLConfig PicPortCLConfig; // Must be filled by child class in constructor
    // PicPortCameraConfig and PicPortCameraInit must be 
    // implemented by every child of this class
    // PicPortCameraConfig should fill SeqDralConfig with
    // camera specific info.
    // PicPortCameraInit should if necessary initialize the
    // camera.
    virtual int PicPortCameraConfig(LvROI &Roi) = 0;
    virtual int PicPortCameraInit() = 0;
    //  This API can be redefined by any driver that want to do processing
    // on a frame before GetFrame returns it to the application.
    virtual Pds::Frame *PicPortFrameProcess(Pds::Frame *pFrame);
    DaSeq32Cfg SeqDralConfig;
    LvROI Roi;
  private:
    static void ReleaseFrame(void *obj, Pds::Frame *pFrame, void *arg);
    int GrabberId;
    DsyApp_Seq32 *pSeqDral;
    enum NotifyType NotifyMode;
    int NotifySignal;
    U8BIT *FrameBufferBaseAddress;
    unsigned long LastFrame;
};

}

#endif	// #ifndef PDS_CAMERA_PICPORTCL
