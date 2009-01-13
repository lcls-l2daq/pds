//! PicPortCL.cc
//! See PicPortCL.hh for a description of what the class can do
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "PicPortCL.hh"
#include <errno.h>
#include <signal.h>

// high = 1 means you want the highest priority signal avaliable, lowest otherwise
extern "C" int __libc_allocate_rtsig (int high);

using namespace PdsLeutron;

PicPortCL::PicPortCL(int _grabberid) {
  DsyInit();
  pSeqDral = NULL;
  NotifySignal = 0;
  NotifyMode = NOTIFYTYPE_NONE;
  GrabberId = _grabberid;
  PicPortCLConfig.bUsePicportCounters = true;
}

PicPortCL::~PicPortCL() {
  if (status.State == CAMERA_RUNNING)
    Stop();
  if (pSeqDral != NULL)
    delete pSeqDral;
  DsyClose();
}

int PicPortCL::DsyToErrno(LVSTATUS DsyError) {
  switch (DsyError) {
    case I_NoError:
      return 0;
    default:
      return EIO;
  };
  return EINVAL;
}

int PicPortCL::SetNotification(enum NotifyType mode) { 
  int ret;
  switch(mode) {
    case NOTIFYTYPE_NONE:
      ret = 0;
      break;
    case NOTIFYTYPE_WAIT:
    case NOTIFYTYPE_SIGNAL:
      if (NotifySignal==0)
	NotifySignal = __libc_allocate_rtsig(1);
      ret = NotifySignal;
      break;
    default:
      ret = -ENOTSUP;
      break;
  }
  if (ret >= 0)
    NotifyMode = mode;
  return ret;
}

int PicPortCL::Init() {
  LVSTATUS lvret;
  int ret;
  LvCameraNode *pCameraNode;
  LvROI LocalRoi;

  // Create the sequencer object. You cannot reconfigure
  // an already configured object, if an old one exist destroy it
  if (pSeqDral != NULL)
    delete pSeqDral;
  pSeqDral = new DsyApp_Seq32;

  // Configure the sequencer, since this part is very dependent 
  // on the Camera we call PicPortCameraConfig which cameras can 
  // re-define.
  lvret = pSeqDral->GetConfig(&SeqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  // Set the framegrabber part of the config
  SeqDralConfig.Size = sizeof(DaSeq32Cfg);
  SeqDralConfig.hGrabber = DsyGetGrabberHandleByName(PICPORTCL_NAME, GrabberId);
  SeqDralConfig.Flags = SqFlg_AutoAdjustBuffer;
  SeqDralConfig.FlowType = SqFlow_PauseOnReq;
  if (NotifyMode != NOTIFYTYPE_NONE) {
    SeqDralConfig.hEvent = NotifySignal;
    SeqDralConfig.Notification = SqNfy_OnEachImage;
  } else {
    SeqDralConfig.Notification = SqNfy_None;
  }
  // Setup the ROI (region Of Interest) to sane values
  LvGrabberNode *vga = DsyGetGrabber(0);
  // Get the sane config from VGA display
  lvret = vga->GetConnectionInfo(vga->GetConnectedMonitor(0), 
                        &LocalRoi);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  LocalRoi.SetTargetBuffer(TgtBuffer_CPU);
  LocalRoi.SetDIBMode(TRUE);
  ret = PicPortCameraConfig(LocalRoi);
  if (ret < 0)
    return ret;
  SeqDralConfig.ConnectCamera.Roi = LocalRoi;

  // Activate the configuration and initialize the frame grabber
  lvret = pSeqDral->SetConfig(&SeqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  lvret = pSeqDral->Activate();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Update configuration with the activated one
  lvret = pSeqDral->GetConfig(&SeqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Initialize the communication serial port to the camera
  LvGrabberNode *pGrabber=DsyGetGrabberPtrFromHandle(SeqDralConfig.hGrabber);
  if (pGrabber == NULL)
    return -EIO;

  SeqDralConfig.hCamera = pGrabber->GetConnectedCamera(0);
  if (SeqDralConfig.hCamera == HANDLE_INVALID) {
    printf("Handle invalid\n");
    return -EINVAL;
  }
  pCameraNode = pGrabber->GetCameraPtr(SeqDralConfig.hCamera);
  if (pCameraNode == NULL)
    return -EIO;
  lvret = pCameraNode->CommOpen();
  if (lvret != I_NoError) {
    printf("comm not open\n");
    return -DsyToErrno(lvret);
  }
  lvret = pCameraNode->CommSetParam(PicPortCLConfig.Baudrate, 
                                  PicPortCLConfig.Parity, 
                                  PicPortCLConfig.ByteSize,
                                  PicPortCLConfig.StopSize
                                  );
  if (lvret != I_NoError) {
    printf("comm set param\n");
    return -DsyToErrno(lvret);
  }
#if 1
  // Now let the Camera configure itself
  ret = PicPortCameraInit();
  if (ret < 0)
    return ret;
#endif
  // Tell the frame grabber to be ready
  lvret = pSeqDral->Init();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  Roi.SetROIInfo(&SeqDralConfig.ConnectCamera.Roi);
  FrameBufferBaseAddress = pSeqDral->GetBufferBaseAddress();
  FrameBufferEndAddress  = FrameBufferBaseAddress + 
    pSeqDral->GetImageOffset(1) * pSeqDral->GetNrImages();

  // We are now ready to acquire frames
  status.State = CAMERA_READY;
  status.CapturedFrames = 0;
  status.DroppedFrames = 0;
  LastFrame = pSeqDral->GetCurrentImage()-1;
  return 0;
}

int PicPortCL::Start() {
  LVSTATUS lvret;

  // Check if Init has been properly called
  if ((status.State != CAMERA_READY) && (status.State != CAMERA_STOPPED))
    return -ENOTCONN;

  // Start the acquisition
  lvret = pSeqDral->Start();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Camera is now running
  status.State = CAMERA_RUNNING;
  return 0;
}

int PicPortCL::Stop() {
  LVSTATUS lvret;

  // Check if Start has been called
  if (status.State != CAMERA_RUNNING)
    return -ENOTCONN;

  // Stop the acquisition
  lvret = pSeqDral->Stop();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Camera is now stopped 
  status.State = CAMERA_STOPPED;
  return 0; 
}

PdsLeutron::FrameHandle *PicPortCL::GetFrameHandle() {
  int ret;
  unsigned long Captured, Current;
  U32BIT StartX, StartY;
  FrameHandle *pFrame;

  // If requested by user we wait for a signal
  if (NotifyMode == NOTIFYTYPE_WAIT) {
    sigset_t sigset;
    int signal;

    sigemptyset(&sigset);
    sigaddset(&sigset, NotifySignal);
    do {
      ret = sigwait(&sigset, &signal);
      if (ret != 0)
        return NULL;
    } while (signal != NotifySignal);
  }

  // Update the status info
  Current = pSeqDral->GetLastAcquiredImage();

  // Read the next frame
  pSeqDral->LockImage(Current);
  if(!pSeqDral->GetImageCoord(Current, &StartX, &StartY))
    return (FrameHandle *)NULL;
  Roi.SetStartPosition(StartX, StartY);

  /*
  LvAcquiredImageInfo imgInfo;
  LvGrabberNode *pGrabber=DsyGetGrabberPtrFromHandle(SeqDralConfig.hGrabber);
  LvCameraNode* pCameraNode = pGrabber->GetCameraPtr(SeqDralConfig.hCamera);
  pCameraNode->GetAcquiredImageInfo(&imgInfo);
  printf("acquired %08d/%08x %d x %d %x\n",
	 imgInfo.FrameId, imgInfo.Timestamp, 
	 imgInfo.AcquiredWidth, imgInfo.AcquiredHeight,
	 imgInfo.Flags);
  */

  if(PicPortCLConfig.bUsePicportCounters) {
    if(Current < LastFrame) 
      Captured = Current - LastFrame + SeqDralConfig.NrImages;
    else
      Captured = Current - LastFrame;
    status.CapturedFrames += Captured;
    if (Captured > 1)
      status.DroppedFrames += Captured - 1;
    LastFrame = Current;
  }
  // Return a Frame object
  pFrame = new FrameHandle(Roi.Width, Roi.Height, config.Format, Roi.PixelIncrement,
        (void *)(FrameBufferBaseAddress + Roi.StartAddress), &PicPortCL::ReleaseFrame, this, (void *)Current);
  return PicPortFrameProcess(pFrame);
}

// For PicPortCL framegrabbers, calling this API will send szCommand on
// the camera link control serial line.
int PicPortCL::SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize) {
  LVSTATUS lvret;
  LvCameraNode *pCameraNode;
  unsigned long ulReceivedLength;
  char ack;
  char sztx[strlen(szCommand)+3];

  // Check if Init has been properly called
  if (!pSeqDral->IsCompiled())
    return -ENOTCONN;
  if ((pszResponse == NULL) || (iResponseBufferSize == 0)) {
    pszResponse = &ack;
    iResponseBufferSize = 1;
  }

  // Get the camera handler
  LvGrabberNode *pGrabber=DsyGetGrabberPtrFromHandle(SeqDralConfig.hGrabber);
  if (pGrabber == NULL)
    return -EIO;
  pCameraNode = pGrabber->GetCameraPtr(SeqDralConfig.hCamera);
  if (pCameraNode == NULL)
    return -EIO;
  
  sztx[0] = PicPortCLConfig.sof;
  strcpy(sztx+1, szCommand);
  sztx[strlen(szCommand)+1] = PicPortCLConfig.eof;
  sztx[strlen(szCommand)+2] = 0;

  // Send the command and receive data
  lvret = pCameraNode->CommSend(sztx, strlen(sztx), pszResponse, 
                iResponseBufferSize, PicPortCLConfig.Timeout_ms, 
                iResponseBufferSize <= 1 ? PicPortCLConfig.eotWrite : PicPortCLConfig.eotRead, 
                (U32BIT *)&ulReceivedLength);

  // Done: if any data was read we return them
  if ((lvret != I_NoError) && (ulReceivedLength == 0))
    return -DsyToErrno(lvret);
  return ulReceivedLength;
}

void PicPortCL::ReleaseFrame(void *obj, FrameHandle *pFrame, void *arg) {
  PicPortCL *pThis = (PicPortCL *)obj;
  // Release last image locked (assume image was processed)
  pThis->pSeqDral->UnlockImage((int)arg);
}

FrameHandle *PicPortCL::PicPortFrameProcess(FrameHandle *pFrame) {
  return pFrame;
}
