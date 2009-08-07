//! PicPortCL.cc
//! See PicPortCL.hh for a description of what the class can do
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/PicPortCL.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/vmon/VmonServerManager.hh"
#include "pds/service/SysClk.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <errno.h>
#include <signal.h>

#define PICPORTCL_CONNECTOR        "CamLink Base Port 0 (HVPSync In 0)"

// high = 1 means you want the highest priority signal avaliable, lowest otherwise
extern "C" int __libc_allocate_rtsig (int high);

namespace PdsLeutron {
  //  Only one thread calls "push", and one other calls "pop"
  class MyQueue {
  public:
    MyQueue(unsigned size) :
      _size(size),
      _head(0),
      _tail(0),
      _contents(new int[size]) { for(unsigned k=0; k<size; k++) _contents[k] = k; }
    ~MyQueue() { delete[] _contents; }
  public:
    inline void push(int v) { _contents[_tail]=v; _tail=(_tail+1)%_size; }
    inline int  pop ()      { int h(_head); _head=(_head+1)%_size; return _contents[h]; }
    unsigned depth(int) const;

    void dump() const 
    { 
      printf("head/tail %d/%d\n",_head,_tail); 
      for(unsigned k=0; k<_size; k++) printf("%d ",_contents[k]); 
      printf("\n"); 
    }
  private:
    int _size;
    int _head;
    int _tail;
    int* _contents;
  };

  unsigned MyQueue::depth(int i) const
  {
    unsigned d = 0;
    int k = _head;
    while( _contents[k]!=i && d<17 ) {
      d++;
      k = (k+1)%_size;
    }
    return d;
  }
};

static int DsyToErrno(LVSTATUS DsyError);
static U16BIT FindConnIndex(HGRABBER hGrabber, U16BIT CameraId, const char* conn);

using namespace PdsLeutron;

PicPortCL::PicPortCL(int grabberid) :
  _pSeqDral(NULL),
  _queue(0)
{
  DsyInit();
  _notifyMode   = NOTIFYTYPE_NONE;
  _notifySignal = 0;
  _grabberId = grabberid;

  Pds::MonGroup* group = new Pds::MonGroup("Cam");
  Pds::VmonServerManager::instance()->cds().add(group);

  Pds::MonDescTH1F depth("Depth", "buffers", "", 17, -0.5, 16.5);
  _depth = new Pds::MonEntryTH1F(depth);
  group->add(_depth);

  float t_ms = float(1<<20)*1.e-6;
  Pds::MonDescTH1F interval("Interval", "[ms]", "", 64, -0.25*t_ms, 31.75*t_ms);
  _interval = new Pds::MonEntryTH1F(interval);
  group->add(_interval);

  Pds::MonDescTH1F interval_w("Interval wide", "[ms]", "", 64, -2.*t_ms, 254.*t_ms);
  _interval_w = new Pds::MonEntryTH1F(interval_w);
  group->add(_interval_w);

  _sample_sec  = 0;
  _sample_nsec = 0;
}

PicPortCL::~PicPortCL() {
  if (status.State == CAMERA_RUNNING)
    Stop();
  if (_pSeqDral != NULL)
    delete _pSeqDral;
  DsyClose();
  if (_queue) delete _queue;
}

int PicPortCL::SetNotification(enum NotifyType mode) { 
  int ret;
  switch(mode) {
    case NOTIFYTYPE_NONE:
      ret = 0;
      break;
    case NOTIFYTYPE_WAIT:
    case NOTIFYTYPE_SIGNAL:
      if (_notifySignal==0)
	_notifySignal = __libc_allocate_rtsig(1);
      ret = _notifySignal;
      break;
    default:
      ret = -ENOTSUP;
      break;
  }
  if (ret >= 0)
    _notifyMode = mode;
  return ret;
}

static void _dumpConfig(const DaSeq32Cfg& cfg)
{
  printf("cfg  size %x  hGrabber %x  hCamera %x  target %x  nrImages %x  seqMode %x  notify %x\n",
	 cfg.Size, cfg.hGrabber, cfg.hCamera, cfg.Target, cfg.NrImages, cfg.SeqMode, cfg.Notification);
  printf("     flags %x  flowtype %x  hEvent %x  flowcheck %x  startx %x  starty %x  evtsignal %x\n",
	 cfg.Flags, cfg.FlowType, cfg.hEvent, cfg.FlowCheck, cfg.StartX, cfg.StartY, cfg.EventSignal);
  printf("     evtEdge %x  evtFunc %x\n",
	 cfg.EventEdge, cfg.EventFunction);
  for(int i=0; i<_Max_InItemCfg; i++) {
    const LvInItemCfg& c = cfg.InOutCfg.In[i];
    const LvSignal& s = c.From.Direct.Signal;
    printf("in %d  Fcn %x  Type %x  Ratio %x  CfgStatus %x  Signal %x/%x/%x\n",
	   i, c.Function, c.Type, c.Ratio, c.CfgStatus, 
	   s.SigId, s.Index, s.Edge);
  }
}

int PicPortCL::Init() {
  LVSTATUS lvret;
  int ret;
  LvCameraNode *pCameraNode;
  LvROI LocalRoi;

  // Create the sequencer object. You cannot reconfigure
  // an already configured object, if an old one exist destroy it
  if (_pSeqDral != NULL)
    delete _pSeqDral;

//   _pSeqDral = (DsyApp_Seq_AsyncReset*)Seq32_AR_CreateObject(const_cast<char*>(Name()),
//  							   (char*)NULL,
//  							   ARST_Mode_0);
  _pSeqDral = trigger_CC1() ?
    (DsyApp_Seq32*)Seq32_AR_CreateObject(const_cast<char*>(Name()),
					 (char*)NULL,
					 ARST_Mode_0) :
    new DsyApp_Seq32;

  // Configure the sequencer, since this part is very dependent 
  // on the Camera we call PicPortCameraConfig which cameras can 
  // re-define.
  lvret = _pSeqDral->GetConfig(&_seqDralConfig);
  //  _dumpConfig(_seqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Set the framegrabber part of the config
  U16BIT CameraId = DsyGetCameraId(const_cast<char*>(Name()));
  _seqDralConfig.Size = sizeof(DaSeq32Cfg);
  _seqDralConfig.hGrabber = DsyGetGrabberHandleByName(PICPORTCL_NAME, _grabberId);
  _seqDralConfig.Flags = SqFlg_AutoAdjustBuffer;
  _seqDralConfig.NrImages = 16;
  //  _seqDralConfig.NrImages = 2;
  _seqDralConfig.UseCameraList = true;
  _seqDralConfig.ConnectCamera.MasterCameraType = CameraId;
  _seqDralConfig.ConnectCamera.NrCamera = 1;
  _seqDralConfig.ConnectCamera.Camera[0].hGrabber = _seqDralConfig.hGrabber;
  _seqDralConfig.ConnectCamera.Camera[0].ConnIndex = 
    FindConnIndex(_seqDralConfig.hGrabber, CameraId, PICPORTCL_CONNECTOR);
  //  The Image Queue is not implemented for Linux!
  //  _seqDralConfig.Flags |= SqFlg_AutoLockFrame;
  //  _seqDralConfig.Flags |= SqFlg_UseImageQueue;

  if (_notifyMode != NOTIFYTYPE_NONE) {
    _seqDralConfig.hEvent = _notifySignal;
    _seqDralConfig.Notification = SqNfy_OnEachImage;
  } else {
    _seqDralConfig.Notification = SqNfy_None;
  }

  // Setup the ROI (region Of Interest) to sane values
  LvGrabberNode *vga = DsyGetGrabber(0);
  // Get the sane config from VGA display
  lvret = vga->GetConnectionInfo(vga->GetConnectedMonitor(0), 
  				 &LocalRoi);

  if (LocalRoi.Height!=pixel_rows() ||
      LocalRoi.Width !=pixel_columns()) {
    printf("Resizing ROI from %d x %d to %d x %d\n",
	   LocalRoi.Width,LocalRoi.Height,
	   pixel_columns(),pixel_rows());
    LocalRoi.Height=pixel_rows();
    LocalRoi.Width =pixel_columns();
  }
  
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  LocalRoi.SetTargetBuffer(TgtBuffer_CPU);
  LocalRoi.SetDIBMode(TRUE);
  switch(output_resolution()) {
  case  8: LocalRoi.SetColorFormat(ColF_Mono_8 ); break;
  case 10: LocalRoi.SetColorFormat(ColF_Mono_10); break;
  case 12: LocalRoi.SetColorFormat(ColF_Mono_12); break;
  default: return -ENOTSUP;
  }
  _seqDralConfig.ConnectCamera.Roi = LocalRoi;

  if (trigger_CC1()) {   //  Use the frame grabber to send the trigger down on CC1
    _seqDralConfig.FlowType      = SqFlow_GoOnReq;
    _seqDralConfig.EventFunction = ExtEvFn_StartAcq;
    _seqDralConfig.EventSignal   = EventSignal_InOutCfg;
    { LvInItemCfg& c = _seqDralConfig.InOutCfg.In[0];
      c.Function = UseAs_FrameTrigger;
      c.Ratio    = 1;
      LvSignal& s = c.From.Direct.Signal;
      s.SigId = SigId_GpioTTL;
      s.Index = 0;
      s.Edge  = IOPol_ActiveHigh; }  // this parameter seems to have no effect
  }
  else {
    _seqDralConfig.FlowType = SqFlow_PauseOnReq;
  }

  // Activate the configuration and initialize the frame grabber
  //  _dumpConfig(_seqDralConfig);
  lvret = _pSeqDral->SetConfig(&_seqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  printf("PicPortCL::Activate()\n");
  lvret = _pSeqDral->Activate();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);
  printf("PicPortCL::Activate() complete\n");

  if (trigger_CC1()) {
    ((DsyApp_Seq_AsyncReset*)_pSeqDral)->SetShutterTime(trigger_duration_us());
    ((DsyApp_Seq_AsyncReset*)_pSeqDral)->SetExternalEvent(ExtEv_Immediate);
  }

  if (_queue) { delete _queue; }
  _queue = new MyQueue(_seqDralConfig.NrImages);

  // Update configuration with the activated one
  lvret = _pSeqDral->GetConfig(&_seqDralConfig);
  //  _dumpConfig(_seqDralConfig);
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  // Initialize the communication serial port to the camera
  LvGrabberNode *pGrabber=DsyGetGrabberPtrFromHandle(_seqDralConfig.hGrabber);
  if (pGrabber == NULL)
    return -EIO;

  _seqDralConfig.hCamera = pGrabber->GetConnectedCamera(0);
  if (_seqDralConfig.hCamera == HANDLE_INVALID) {
    printf("Handle invalid\n");
    return -EINVAL;
  }
  pCameraNode = pGrabber->GetCameraPtr(_seqDralConfig.hCamera);
  if (pCameraNode == NULL)
    return -EIO;
  lvret = pCameraNode->CommOpen();
  if (lvret != I_NoError) {
    printf("comm not open\n");
    return -DsyToErrno(lvret);
  }
  lvret = pCameraNode->CommSetParam(baudRate(),
				    parity(),
				    byteSize(),
				    stopSize());
  if (lvret != I_NoError) {
    printf("comm set param\n");
    return -DsyToErrno(lvret);
  }
  // Now let the Camera configure itself
  ret = PicPortCameraInit();
  if (ret < 0)
    return ret;
  // Tell the frame grabber to be ready
  lvret = _pSeqDral->Init();
  if (lvret != I_NoError)
    return -DsyToErrno(lvret);

  _roi.SetROIInfo(&_seqDralConfig.ConnectCamera.Roi);
  _frameBufferBase = frameBufferBaseAddress();

  switch(output_resolution()) {
  case  8: _frameFormat = FrameHandle::FORMAT_GRAYSCALE_8 ; break;
  case 10: _frameFormat = FrameHandle::FORMAT_GRAYSCALE_10; break;
  case 12: _frameFormat = FrameHandle::FORMAT_GRAYSCALE_12; break;
  }

  // We are now ready to acquire frames
  status.State = CAMERA_READY;
  return 0;
}

unsigned char* PicPortCL::frameBufferBaseAddress() const { return _pSeqDral->GetBufferBaseAddress(); }

unsigned char* PicPortCL::frameBufferEndAddress () const 
{
  return _pSeqDral->GetBufferBaseAddress() +
    _pSeqDral->GetImageOffset(1) * _pSeqDral->GetNrImages();
}

int PicPortCL::Start() {
  LVSTATUS lvret;

  // Check if Init has been properly called
  if ((status.State != CAMERA_READY) && (status.State != CAMERA_STOPPED))
    return -ENOTCONN;

  // Start the acquisition
  if ((lvret=_pSeqDral->Start()) != I_NoError)
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
  if ((lvret=_pSeqDral->Stop()) != I_NoError)
    return -DsyToErrno(lvret);

  // Camera is now stopped 
  status.State = CAMERA_STOPPED;
  return 0; 
}

FrameHandle *PicPortCL::GetFrameHandle() {
  int ret;
  U32BIT StartX, StartY;
  FrameHandle *pFrame;

  // If requested by user we wait for a signal
  if (_notifyMode == NOTIFYTYPE_WAIT) {
    sigset_t sigset;
    int signal;

    sigemptyset(&sigset);
    sigaddset(&sigset, _notifySignal);
    do {
      ret = sigwait(&sigset, &signal);
      if (ret != 0)
        return NULL;
    } while (signal != _notifySignal);
  }

  { timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    unsigned dsec(tp.tv_sec - _sample_sec);
    if (dsec<2) {
      unsigned nsec(tp.tv_nsec);
      if (dsec==1) nsec += 1000000000;
      unsigned b = (nsec-_sample_nsec)>>19;
      if (b < _interval->desc().nbins())
	_interval->addcontent(1.,b);
      else
	_interval->addinfo(1.,Pds::MonEntryTH1F::Overflow);
      b >>= 3;
      if (b < _interval_w->desc().nbins())
	_interval_w->addcontent(1.,b);
      else
	_interval_w->addinfo(1.,Pds::MonEntryTH1F::Overflow);
    }
    _sample_sec  = tp.tv_sec;
    _sample_nsec = tp.tv_nsec;

    _depth->addcontent(1.,_queue->depth(_pSeqDral->GetLastAcquiredImage()));

    Pds::ClockTime clock(tp.tv_sec,tp.tv_nsec);
    _interval_w->time(clock);
    _interval  ->time(clock);
    _depth     ->time(clock);
  }

  // Read the next frame
  unsigned current = _queue->pop();
  _pSeqDral->LockImage(current);

  if(!_pSeqDral->GetImageCoord(current, &StartX, &StartY)) {
    printf("*** PPCL GetImageCoord failed frame %u  Start %d,%d\n",
	   current,StartX,StartY);
    return (FrameHandle *)NULL;
  }
  _roi.SetStartPosition(StartX, StartY);

  // Return a Frame object
  pFrame = new PdsLeutron::FrameHandle(_roi.Width, _roi.Height, 
				       _frameFormat, _roi.PixelIncrement,
				       (void *)(_frameBufferBase + _roi.StartAddress), 
				       &PicPortCL::ReleaseFrame, 
				       this, (void *)current);
  return PicPortFrameProcess(pFrame);
}

// For PicPortCL framegrabbers, calling this API will send szCommand on
// the camera link control serial line.
int PicPortCL::SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize) {
  LVSTATUS lvret;
  LvCameraNode *pCameraNode;
  U32BIT ulReceivedLength;
  char ack;
  char sztx[strlen(szCommand)+3];

  // Check if Init has been properly called
  if (!_pSeqDral->IsCompiled())
    return -ENOTCONN;
  if ((pszResponse == NULL) || (iResponseBufferSize == 0)) {
    pszResponse = &ack;
    iResponseBufferSize = 1;
  }

  // Get the camera handler
  LvGrabberNode *pGrabber=DsyGetGrabberPtrFromHandle(_seqDralConfig.hGrabber);
  if (pGrabber == NULL)
    return -EIO;
  pCameraNode = pGrabber->GetCameraPtr(_seqDralConfig.hCamera);
  if (pCameraNode == NULL)
    return -EIO;
  
  sztx[0] = sof();
  strcpy(sztx+1, szCommand);
  sztx[strlen(szCommand)+1] = eof();
  sztx[strlen(szCommand)+2] = 0;

  // Send the command and receive data
  lvret = pCameraNode->CommSend(sztx, strlen(sztx), pszResponse, 
				iResponseBufferSize, timeout_ms(), 
				iResponseBufferSize <= 1 ? eotWrite() : eotRead(), 
				&ulReceivedLength);

  // Done: if any data was read we return them
  if ((lvret != I_NoError) && (ulReceivedLength == 0))
    return -DsyToErrno(lvret);
  return ulReceivedLength;
}

void PicPortCL::ReleaseFrame(void *obj, FrameHandle *pFrame, void *arg) {
  PicPortCL *pThis = (PicPortCL *)obj;
  // Release last image locked (assume image was processed)
  pThis->_pSeqDral->UnlockImage((size_t)arg);
  pThis->_queue->push((size_t)arg);
}

FrameHandle *PicPortCL::PicPortFrameProcess(FrameHandle *pFrame) {
  return pFrame;
}


//===============================
//  Static functions
//===============================

int DsyToErrno(LVSTATUS DsyError) {
  switch (DsyError) {
    case I_NoError:
      return 0;
    default:
      return EIO;
  };
  return EINVAL;
}

// ========================================================
// FindConnIndex
// ========================================================
// Utility function to find connector index (if exists)
// based on the connector name and previously found
// grabber and camera IDs.
// ========================================================
U16BIT FindConnIndex(HGRABBER hGrabber, U16BIT CameraId, const char* conn) {
  U16BIT ConnIndex = HANDLE_INVALID;
  LvGrabberNode *Grabber=DsyGetGrabberPtrFromHandle(hGrabber);
  int NrFree=Grabber->GetNrFreeConnectorEx(CameraId);

  if (NrFree) {
    LvConnectionInfo *ConnectorInfo=new LvConnectionInfo[NrFree];
    // Copy the info to the array
    Grabber->GetFreeConnectorInfoEx(CameraId, ConnectorInfo);
    // Check if it is possible to connect the camera to the
    // grabber via the specified connector.
    for(U16BIT Index=0; Index<NrFree; Index++) {
      if (strncmp(ConnectorInfo[Index].Description, conn, strlen(conn))==0) {
        ConnIndex = Index;
        break;
      }
    }
  }
  return ConnIndex;
}

void PicPortCL::dump()  
{
  if (_queue) {
    _queue->dump();
  }
}
