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

#include "pds/camera/LvCamera.hh"

#define PICPORTCL_NAME    "PicPortX CL Mono"

namespace Pds {
  class MonEntryTH1F;
};

namespace PdsLeutron {

  class FrameHandle;
  class MyQueue;

  class PicPortCL: public LvCamera {
  public:
    PicPortCL(int _grabberid = 0);
    virtual ~PicPortCL();
  public:
    int SetNotification(enum NotifyType mode);
    int Init();
    int Start();
    int Stop();
    FrameHandle *GetFrameHandle();
    int SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize);
    unsigned char* frameBufferBaseAddress() const;
    unsigned char* frameBufferEndAddress () const;

    void dump();
  private:
    //  Serial command interface
    virtual int           baudRate() const=0;
    virtual unsigned long parity() const=0;
    virtual unsigned long byteSize() const=0;
    virtual unsigned long stopSize() const=0;
    virtual char          eotWrite() const=0;
    virtual char          eotRead() const=0;
    virtual char          sof() const=0;
    virtual char          eof() const=0;
    virtual unsigned long timeout_ms() const=0;
  private:
    virtual const char* Name() const = 0;
    virtual bool        trigger_CC1        () const = 0;
    virtual unsigned    trigger_duration_us() const = 0;
    virtual unsigned    output_resolution  () const = 0;
    virtual unsigned    pixel_rows         () const = 0;
    virtual unsigned    pixel_columns      () const = 0;
    // PicPortCameraInit should if necessary initialize the camera.
    virtual int PicPortCameraInit() = 0;
    //  This API can be redefined by any driver that want to do processing
    // on a frame before GetFrame returns it to the application.
    virtual FrameHandle *PicPortFrameProcess(FrameHandle *pFrame);
  private:
    static void ReleaseFrame(void *obj, FrameHandle *pFrame, void *arg);
  private:
    DaSeq32Cfg             _seqDralConfig;
    LvROI                  _roi;
    int                    _grabberId;
    //    DsyApp_Seq_AsyncReset* _pSeqDral;
    DsyApp_Seq32*          _pSeqDral;
    MyQueue*               _queue;
  private:
    enum NotifyType _notifyMode;
    int             _notifySignal;
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
