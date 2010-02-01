#include "PrincetonServer.hh" 
#include "PrincetonUtils.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"

#include <errno.h>
#include <pthread.h> 
 
using std::string;

using PICAM::printPvError;

namespace Pds 
{

using namespace Princeton;

PrincetonServer::PrincetonServer(bool bUseCaptureThread, bool bStreamMode, std::string sFnOutput, const Src& src, int iDebugLevel) :
 _bUseCaptureThread(bUseCaptureThread), _bStreamMode(bStreamMode), _sFnOutput(sFnOutput), _src(src), _iDebugLevel(iDebugLevel),
 _hCam(-1), _iCurShotIdStart(-1), _iCurShotIdEnd(-1), _fReadoutTime(0), _configCamera(), 
 _uFrameSize(0), _pCircBufferWithHeader(NULL), _iCurBufferIndex(0), 
 _iCameraAbortAndReset(0), _iEventCaptureEnd(0), _bForceCameraReset(false), _iTemperatureStatus(0), 
 _dgEvent(TypeId(), Src()), _pDgOut(NULL)
{       
  if ( checkInitSettings() != 0 )    
    throw PrincetonServerException( "PrincetonServer::PrincetonServer(): Invalid initial settings" );
      
  if ( initCamera() != 0 )
    throw PrincetonServerException( "PrincetonServer::PrincetonServer(): initPrincetonCamera() failed" );    
  
  if ( initControlThreads() != 0 )
    throw PrincetonServerException( "PrincetonServer::PrincetonServer(): initControlThreads() failed" );
}

PrincetonServer::~PrincetonServer()
{
  deinitCamera();
}

int PrincetonServer::initCamera()
{
  /* Initialize the PVCam Library */
  if (!pl_pvcam_init())
  {
    printPvError("PrincetonServer::initCamera(): pl_pvcam_init() failed");
    return 1;
  }

  char strCamera[CAM_NAME_LEN];  /* camera name                    */
  
  /* Assume each machine only control one camera */
  if (!pl_cam_get_name(0, strCamera))
  {
    printPvError("PrincetonServer::initCamera(): pl_cam_get_name(0) failed");
    return 2;
  }
  
  if (!pl_cam_open(strCamera, &_hCam, OPEN_EXCLUSIVE))
  {
    printPvError("PrincetonServer::initCamera(): pl_cam_open() failed");
    return 3;
  }
    
  return 0;
}

int PrincetonServer::deinitCamera()
{  
  // If the camera has been configured before, and not un-configured yet  
  if ( _pCircBufferWithHeader != NULL )
    unconfigCamera(); // un-configure the camera explicitly    
    
  if (!pl_cam_close(_hCam))
    printPvError("PrincetonServer::deinitCamera(): pl_cam_close() failed"); // Don't return here; continue to uninit the library

  if (!pl_pvcam_uninit())
  {
    printPvError("PrincetonServer::deinitCamera(): pl_pvcam_uninit() failed");
    return 2;
  }
  
  return 0;
}

int PrincetonServer::configCamera(Princeton::ConfigV1& config)
{
  // If the camera has been configured before, and not un-configured yet
  //   Note: This happens when PrincetonManager missed an Unconfigure event between two run.
  if ( _pCircBufferWithHeader != NULL )
    unconfigCamera(); // un-configure the camera explicitly  
  
  int iFail;
  iFail= initCameraSettings(config);  
  if ( iFail != 0 ) return 1;
  
  _configCamera = config;
  FrameV1::initStatic( _configCamera ); // associate frame objects with this camera config
    
  iFail = setupCooling();  
  if ( iFail != 0 ) return 2;  
  
  iFail = initCapture();
  if ( iFail != 0 ) return 3; 
  
  return 0;
}

int PrincetonServer::unconfigCamera()
{
  /* Stop the acquisition */
  if (!pl_exp_abort(_hCam, CCS_HALT))
    printPvError("PrincetonServer::unconfigCamera():pl_exp_abort() failed");
  
  /* Uninit the sequence */
  if (!pl_exp_uninit_seq()) 
    printPvError("PrincetonServer::unconfigCamera():pl_exp_uninit_seq() failed");
  
  free(_pCircBufferWithHeader);
  _pCircBufferWithHeader = NULL;
  
  return 0;
}

int PrincetonServer::initCapture()
{ 
  rgn_type region;  
  setupROI(region);
  PICAM::printROI(1, &region);

  if (!pl_exp_init_seq())
  {
    printPvError("PrincetonServer::initCapture(): pl_exp_init_seq() failed!\n");
    return 1; 
  }
 
  const int16 iExposureMode = _configCamera.exposureMode();
  
  /*
   * _configCamera.exposureTime() time is measured in seconds,
   *  while iExposureTime for pl_exp_setup() is measured in milliseconds
   *
   * Note: this value will be ignored by pl_exp_setup(), if exposure mode = BULB_MODE
   */
  const uns32 iExposureTime = (int) ( _configCamera.exposureTime() * 1000 ); 
  
  rs_bool bStatus = 
   pl_exp_setup_seq(_hCam, 1, 1, &region, iExposureMode, iExposureTime, &_uFrameSize);
  if (!bStatus)
  {
    printPvError("PrincetonServer::initCapture(): pl_exp_setup_seq() failed!\n");
    return 2;
  }
  printf( "Frame size for image capture = %lu\n", _uFrameSize );  

  /* Set up a circular buffer */
  
  _pCircBufferWithHeader = (unsigned char *) calloc(_iFrameHeaderSize + _uFrameSize, _iCircBufFrameCount);
  if (!_pCircBufferWithHeader)
  {
    printf("PrincetonServer::initCapture(): Memory allocation error!\n");
    return 4;
  }
  
  _iCurBufferIndex = 0; // reset the frame index to 0 
  
  int iFail = 
   startCapture(0);
  if ( iFail != 0 ) return 5;
   
  return 0;
}

int PrincetonServer::startCapture(int iBufferIndex)
{  
  unsigned char* pFrameBuffer = _pCircBufferWithHeader + iBufferIndex * (_iFrameHeaderSize + _uFrameSize) + _iFrameHeaderSize;  
  
  /* Start the acquisition */
  rs_bool bStatus = 
   pl_exp_start_seq(_hCam, pFrameBuffer);
  if (!bStatus)
  {
    printPvError("PrincetonServer::startCapture():pl_exp_start_seq() failed");
    return 1;        
  }  
  
  return 0;
}

int PrincetonServer::initCameraSettings(Princeton::ConfigV1& config)
{
  if ( config.orgX() + config.width() > Princeton::ConfigV1::uPI_MTE_2048_Width 
    || config.orgY() + config.height() > Princeton::ConfigV1::uPI_MTE_2048_Height
    || config.binX() >= Princeton::ConfigV1::uPI_MTE_2048_Width / 32 
    || config.binY() >= Princeton::ConfigV1::uPI_MTE_2048_Height / 32 )
  {
    printf( "PrincetonServer::validateCameraSettings(): Config parameters are invalid\n" );
    return 1;
  }
  
  uns32 uTriggerEdge = EDGE_TRIG_POS;
  PICAM::setAnyParam(_hCam, PARAM_EDGE_TRIGGER, &uTriggerEdge );  
  
  using PICAM::displayParamIdInfo;
  displayParamIdInfo(_hCam, PARAM_EXPOSURE_MODE,    "Exposure Mode");
  displayParamIdInfo(_hCam, PARAM_CLEAR_MODE,       "Clear Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_MODE,   "Shutter Open Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_DELAY,  "Shutter Open Delay");
  displayParamIdInfo(_hCam, PARAM_SHTR_CLOSE_DELAY, "Shutter Close Delay");
    
  displayParamIdInfo(_hCam, PARAM_EXP_RES,          "Exposure Resolution");
  displayParamIdInfo(_hCam, PARAM_EXP_RES_INDEX,    "Exposure Resolution Index");
  
  displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER,     "Edge Trigger" );
  
  return 0;
}

int PrincetonServer::setupCooling()
{
  using namespace PICAM;
  // Cooling settings
  
  displayParamIdInfo(_hCam, PARAM_COOLING_MODE, "Cooling Mode");

  //displayParamIdInfo(_hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature *Org*");  
  const int16 iCoolingTemp = _configCamera.coolingTemp();
  setAnyParam(_hCam, PARAM_TEMP_SETPOINT, &iCoolingTemp );
  displayParamIdInfo(_hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature" );   

  displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature Before Cooling" );  
  
  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 millisecond    
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );      
  
  int16 iTemperatureCurrent = -1;  
  int iNumLoop = 0;
  
  while (1)
  {  
    getAnyParam(_hCam, PARAM_TEMP, &iTemperatureCurrent );
    if ( iTemperatureCurrent <= iCoolingTemp ) break;
    
    if ( (iNumLoop+1) % 1000 == 0 )
      displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature *Updating*" );
    
    if ( iNumLoop > _iMaxCoolingTime ) break;
    
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);    
    iNumLoop++;
  }
  
  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fCoolingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;    
  printf("Cooling Time = %6.3lf ms\n", fCoolingTime);  
  
  displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature After Cooling" );
  
  if ( iTemperatureCurrent > iCoolingTemp ) 
  {
    printf("PrincetonServer::setupCooling(): Cooling failed, final temperature = %lf degree Celcius", 
     (iTemperatureCurrent / 100.0) );
    return 1;
  }
  
  return 0;
}

int PrincetonServer::initControlThreads()
{
  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

  pthread_t threadControl;
  
  int iFail;
  
  iFail = 
   pthread_create(&threadControl, &threadAttr, &threadEntryMonitor, this);
  if (iFail != 0)
  {
    printf("PrincetonServer::initControlThreads(): pthread_create(threadEntryMonitor) failed, error code %d - %s\n",
       errno, strerror(errno));
    return 1;
  }

  if ( ! _bUseCaptureThread )
    return 0;
    
  iFail = 
   pthread_create(&threadControl, &threadAttr, &threadEntryCapture, this);   
  if (iFail != 0)
  {
    printf("PrincetonServer::initControlThreads(): pthread_create(threadEntryCapture) failed, error code %d - %s\n",
       errno, strerror(errno));
    return 1;
  }
  
  return 0;
}

int PrincetonServer::runMonitorThread()
{
  const static timeval timeSleepMicroOrg = {0, 100000}; // 100 milliseconds
  timespec tsPrevIdle;  
  
  while (1)
  { 
    /*
     * Sleep for 100ms
     */
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro); 

    /*
     * Check the rest request
     */    
    if ( _bForceCameraReset ) // request of reset, sent from the other thread
    {
      abortAndResetCamera();      
      _bForceCameraReset = false;      
    }    
    
    /*
     * Check temperature
     */
    checkTemperature();   

    /*
     * If system is idle, proceed to next iteration
     */
    if ( isExposureInProgress() == 0 )
    {
      clock_gettime( CLOCK_REALTIME, &tsPrevIdle );
      continue;
    }
     
    /*
     * Check if the exposure time exceeds the limit
     */    
    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );
    
    int iCaptureTime = (tsCurrent.tv_nsec - tsPrevIdle.tv_nsec) / 1000000 + 
     ( tsCurrent.tv_sec - tsPrevIdle.tv_sec ) * 1000; // in ms
    if ( iCaptureTime >= _iMaxExposureTime )
    {
      printf( "PrincetonServer::runMonitorThread(): Exposure time is too long. Capture is stopped\n" );          
      abortAndResetCamera();
      clock_gettime( CLOCK_REALTIME, &tsPrevIdle );
    }
  }
  return 0;
}

int PrincetonServer::runCaptureThread()
{
  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds
  CDatagram cdgEvent(_dgEvent);
  
  while (1)
  { 
    // !! use condition variable
    if ( !_iEventCaptureEnd )
    {
      // This data will be modified by select(), so need to be reset
      timeval timeSleepMicro = timeSleepMicroOrg; 
      // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
      select( 0, NULL, NULL, NULL, &timeSleepMicro);      
      continue;      
    }
    
    if ( _iCameraAbortAndReset != 0 )
    {
      _iCurShotIdEnd    = -1;  
      _pDgOut           = NULL;      
      _iEventCaptureEnd = 0;
      continue;
    }    
    
    // if shot Ids have not been correctly updated
    if ( _iCurShotIdStart < 0 || _iCurShotIdEnd < 0 )
    {      
      _iEventCaptureEnd = 0;
      continue;
    }
        
    new (&cdgEvent) CDatagram(_dgEvent);
    InDatagram* in  = &cdgEvent;
    InDatagram* out = in;
    
    int iFail;
    iFail =
     waitForNewFrameAvailable();
     
    if ( iFail == 0 )
    {
      iFail = 
       processFrame(in, out);       
    }
          
    /*
     * Set the damage bit if temperature status is not good
     */
    if ( iFail != 0 || _iTemperatureStatus != 0 )
      out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    
    if ( _bStreamMode )
      _pDgOut = out;
    else
      writeFrameToFile( out->datagram() );
    
    /* 
     * Reset the shot-by-shot data
     */
    _iCurShotIdStart = _iCurShotIdEnd = -1;      
    
    _iEventCaptureEnd = 0;
  }
  return 0;
}

int PrincetonServer::onEventShotIdStart(int iShotIdStart)
{

  if ( _iCameraAbortAndReset == 1 )
  {
    printf( "PrincetonServer::onEventShotIdStart(): Camera resetting. No data is outputted\n" );       
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 1;   
  }
  
  if ( _iCameraAbortAndReset == 2 )
  {
    _iCurShotIdEnd = -1;
    _iCameraAbortAndReset = 0;
  }
    
  if ( _iEventCaptureEnd == 1 || _pDgOut != NULL )
  {
    printf( "PrincetonServer::onEventShotIdStart(): Last frame has not been processed completely, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );     
    // Don't reset the shot id, because the capture thread should be processing the data
    return 2;
  }
  
  if ( _iCurShotIdStart >= 0 || _iCurShotIdEnd >= 0 )
  {
    printf( "PrincetonServer::onEventShotIdStart(): Shot Id (Start or End) has not been reset since the last shot, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );    

    // Don't reset the shot id, if the capture thread might be processing the data; otherwise always reset the shot ids     
    if (!_bUseCaptureThread)
      _iCurShotIdStart = _iCurShotIdEnd = -1;    
    return 3;
  }    
    
  _iCurShotIdStart = iShotIdStart;  
    
  return 0;
}

int PrincetonServer::onEventShotIdEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out)
{
  out = in; // default: return empty stream
  
  if ( _iCameraAbortAndReset != 0 )
  {
    printf( "PrincetonServer::onEventShotIdEnd(): Camera resetting. No data is outputted\n" );   
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 1;
  }
    
  if ( _iCurShotIdStart == -1 )
  {
    printf( "PrincetonServer::onEventShotIdEnd(): No capture start event was received before the capture stop event\n" );
    _iCurShotIdEnd = -1;
    return 2;
  }  

  if ( _iEventCaptureEnd == 1 || _pDgOut != NULL )
  {
    printf( "PrincetonServer::onEventShotIdEnd(): Last frame has not been processed completely, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );     
    // Don't reset the shot id, because the capture thread should be processing the data, or the data is waiting to be read out
    return 3;
  }
  
  if ( _iCurShotIdEnd >= 0 )
  {
    printf( "PrincetonServer::onEventShotIdEnd(): Shot Id (End) has not been reset since the last shot, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );    
     
    // Don't reset the shot id, if the capture thread might be processing the data; otherwise always reset the shot ids
    if (!_bUseCaptureThread)
      _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 4;
  }
    
  if ( _bUseCaptureThread )
  {
    /*
     * _bUseCaptureThread = true -> Use asynchronous capture thread to get the image data and return later
     */    
    _iCurShotIdEnd    = iShotIdEnd;    
    _dgEvent          = in->datagram();
    _iEventCaptureEnd = 1;  // Use event trigger "_iEventCaptureEnd" to notify the thread
    return 0;
  }
    
  /*
   * _bUseCaptureThread = false -> Use blocking functions to get the image data and return
   */
   
  if ( waitForNewFrameAvailable() != 0 )
  {
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 5;
  }
      
  _iCurShotIdEnd = iShotIdEnd;
  if ( processFrame( in, out ) != 0 )
  {
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 6;
  }

  /* 
   * Reset the shot-by-shot data
   */
  _iCurShotIdStart = _iCurShotIdEnd = -1;
    
  if ( _iTemperatureStatus != 0 )  
    return 7;    
      
  return 0;
}

int PrincetonServer::onEventShotIdUpdate(int iShotIdStart, int iShotIdEnd, InDatagram* in, InDatagram*& out)
{
  out = in; // default: return empty stream
  
  if ( _iCameraAbortAndReset == 1 )
  {
    printf( "PrincetonServer::onEventShotIdUpdate(): Camera resetting. No data is outputted\n" );   
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 1;
  }
  
  if ( _iCameraAbortAndReset == 2 )
  {
    printf( "PrincetonServer::onEventShotIdUpdate(): Camera has been reset. Skip the current data\n" );
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    _iCameraAbortAndReset = 0;
    return 2;
  }  

  if ( _iEventCaptureEnd == 1 || _pDgOut != NULL )
  {
    printf( "PrincetonServer::onEventShotIdUpdate(): Last frame has not been processed completely, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );     
    // Don't reset the shot id, because the capture thread should be processing the data, or the data is waiting to be read out
    return 3;
  }
  
  if ( _iCurShotIdEnd >= 0 )
  {
    printf( "PrincetonServer::onEventShotIdUpdate(): Shot Id (End) has not been reset since the last shot, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );    
     
    // Don't reset the shot id, if the capture thread might be processing the data; otherwise always reset the shot ids
    if (!_bUseCaptureThread)
      _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 4;
  }
    
  if ( _bUseCaptureThread )
  {
    /*
     * _bUseCaptureThread = true -> Use asynchronous capture thread to get the image data and return later
     */    
    _iCurShotIdStart  = iShotIdStart;
    _iCurShotIdEnd    = iShotIdEnd;    
    _dgEvent          = in->datagram();
    _iEventCaptureEnd = 1;  // Use event trigger "_iEventCaptureEnd" to notify the thread
    return 0;
  }
    
  /*
   * _bUseCaptureThread = false -> Use blocking functions to get the image data and return
   */
   
  if ( waitForNewFrameAvailable() != 0 )
  {
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 5;
  }
      
  _iCurShotIdStart  = iShotIdStart;  
  _iCurShotIdEnd    = iShotIdEnd;
  if ( processFrame( in, out ) != 0 )
  {
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 6;
  }

  /* 
   * Reset the shot-by-shot data
   */
  _iCurShotIdStart = _iCurShotIdEnd = -1;
    
  if ( _iTemperatureStatus != 0 )  
    return 7;    
      
  return 0;
}

int PrincetonServer::getMakeUpData(InDatagram* in, InDatagram*& out)
{
  out = in; // default: return empty stream

  /*
   * This function is only useful when _bUseCaptureThread = true and bStreamMode = true
   */
  if ( !_bUseCaptureThread || !_bStreamMode ) 
    return 1;  
  
  if ( _iCameraAbortAndReset != 0 )
    return 1;
  
  if ( _pDgOut == NULL )
    return 0;
    
  Datagram& dgIn    = in->datagram();
  Datagram& dgOut   = _pDgOut->datagram();
  Xtc       xtcOrg  = dgOut.xtc;
  
  /*
   * Compose the datagram
   *
   *   1. Use the header from dgIn 
   *   2. Use the xtc (data header) from dgOut
   *   3. Use the data from dgOut (the data was located after the xtc)
   */
  dgOut = dgIn;   
  dgOut.xtc = xtcOrg;
  
  out = _pDgOut;
  
  // After sending out the data, set the _pDgOut pointer to NULL, so that the same data will never be sent out twice
  _pDgOut = NULL;
  
  return 0;
}

void PrincetonServer::abortAndResetCamera()
{    
  _iCameraAbortAndReset = 1;
  
  printf( "PrincetonServer::abortAndResetCamera(): Camera reset begins.\n" );
  
  int iFail;
  iFail = unconfigCamera();
  if ( iFail != 0 )
  {
    /* Unconfigure failed. Force camera to abort the acquisition */
    if (!pl_exp_abort(_hCam, CCS_HALT))
    {
      printPvError("PrincetonServer::abortAndResetCamera():pl_exp_abort() failed");
      
      /* 
       * Abort action failed. Manually de-init the camera and init again
       */           
      deinitCamera();
      iFail = initCamera();
      if ( iFail != 0 )
        abort();
    }    
  }
    
  iFail = initCapture();
  if ( iFail != 0 )
    abort();
    
  printf( "PrincetonServer::abortAndResetCamera(): Camera reset ends.\n" );
  
  _iCameraAbortAndReset = 2; // The event thread ( onEventShotIdStart() ) will reset this value back to 0
}

int PrincetonServer::isExposureInProgress()
{  
  int16 iStatus = 0;
  uns32 uNumBytesTransfered;  
  rs_bool bCheckOk;
  
  lockPlFunc();
  bCheckOk = 
   pl_exp_check_status(_hCam, &iStatus, &uNumBytesTransfered);
  releaseLockPlFunc();  
  
  if (!bCheckOk)   
  {
    if ( pl_error_code() == 0 )
      return 0;
      
    printPvError("PrincetonServer::isExposureInProgress(): pl_exp_check_status() failed\n");
    return 0;
  }
        
  /*
   * READOUT_COMPLETE=FRAME_AVAILABLE, according to PVCAM 2.7 library manual
   */
  if (iStatus == READOUT_COMPLETE) 
    return 0;
    
  if (iStatus == READOUT_NOT_ACTIVE) // system in idle state
    return 0;

  if (iStatus == READOUT_IN_PROGRESS) // system is reading out the data, but not doing the exposure
    return 0;
    
  /*
   * Reutrn 1 for the remaining cases:
   * EXPOSURE_IN_PROGRESS, ACQUISITION_IN_PROGRESS
   */
   
  return 1;
}

int PrincetonServer::waitForNewFrameAvailable()
{  
  static timespec tsWaitStart;
  clock_gettime( CLOCK_REALTIME, &tsWaitStart );
  
  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds
  
  while (1)
  {
    int16 iStatus = 0;
    uns32 uNumBytesTransfered;
    rs_bool bCheckOk;
    
    lockPlFunc();  
    bCheckOk = 
     pl_exp_check_status(_hCam, &iStatus, &uNumBytesTransfered);
    releaseLockPlFunc();  
    
    if (!bCheckOk)
    {
      if ( pl_error_code() == 0 )
        return 1;
      
      printPvError("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() failed\n");
      return 1;
    }
    
    /*
     * READOUT_COMPLETE=FRAME_AVAILABLE, according to PVCAM 2.7 library manual
     */
    if (iStatus == READOUT_COMPLETE) // New frame available
      break;

    if (iStatus == READOUT_FAILED)
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() return status=READOUT_FAILED\n");      
      return 2;
    }

    if (iStatus == READOUT_NOT_ACTIVE) // idle state: camera controller won't transfer any data
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() return status=READOUT_NOT_ACTIVE\n");      
      return 3;
    }
    
    /*
     * sleep and do the next iteration, for the cases:
     * 
     * READOUT_IN_PROGRESS, EXPOSURE_IN_PROGRESS, ACQUISITION_IN_PROGRESS
     */
     
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro); 
        
          
    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );
    
    int iWaitTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1000000 + 
     ( tsCurrent.tv_sec - tsWaitStart.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime >= _iMaxReadoutTime )
    {
      printf( "PrincetonServer::waitForNewFrameAvailable(): Readout time is too long. Capture is stopped\n" );          
      _bForceCameraReset = true;
      return 4;
    }     
  } // while (1)

  timespec tsWaitEnd;
  clock_gettime( CLOCK_REALTIME, &tsWaitEnd );
  
  _fReadoutTime = (tsWaitEnd.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsWaitEnd.tv_sec - tsWaitStart.tv_sec ); // in seconds
  
  int iNextBufferIndex = ( _iCurBufferIndex + 1 ) % _iCircBufFrameCount;
  startCapture( iNextBufferIndex );
  
  return 0;
}

int PrincetonServer::processFrame(InDatagram* in, InDatagram*& out)
{  
  // default to return the empty (original) datagram without data
  out = in; 

  /*
   * Set frame object
   */    
  unsigned char* pCDatagram   = _pCircBufferWithHeader + _iCurBufferIndex * (_iFrameHeaderSize + _uFrameSize);    
  unsigned char* pXtcHeader   = pCDatagram + sizeof(CDatagram);
  unsigned char* pFrameHeader = pXtcHeader + sizeof(Xtc);  
          
  new (pFrameHeader) FrameV1(_iCurShotIdStart, _iCurShotIdEnd, _fReadoutTime);

  TypeId typePrincetonFrame(TypeId::Id_PrincetonFrame, FrameV1::Version);
  Xtc* pXtcFrame = 
   new ((char*)pXtcHeader) Xtc(typePrincetonFrame, _src);
  pXtcFrame->alloc( FrameV1::size() );
    
  out = 
   new (pCDatagram) CDatagram( in->datagram() ); 
  out->datagram().xtc.alloc( sizeof(Xtc) + FrameV1::size() );
  
  /*
   * Update current buffer index
   */
  _iCurBufferIndex = ( _iCurBufferIndex + 1 ) % _iCircBufFrameCount;       
    
  return 0;
}

int PrincetonServer::writeFrameToFile(const Datagram& dgOut)
{
  // File mode is not supported yet
  return 0;
}

int PrincetonServer::checkInitSettings()
{
  /* Check the input settings */
  if ( !_bStreamMode )
  {
    printf( "PrincetonServer::onEventShotIdEnd(): File mode is not supported.\n" );
    return 1;
  }
  
  if ( !_bStreamMode && !_bUseCaptureThread )  
  {
    printf( "PrincetonServer::onEventShotIdEnd(): Unsupported mode: File mode without capture thread.\n" );
    return 2;
  }
      
  return 0;
}

void PrincetonServer::setupROI(rgn_type& region)
{
  region.s1   = _configCamera.orgX();
  region.s2   = _configCamera.orgX() + _configCamera.width() - 1;
  region.sbin = _configCamera.binX();
  region.p1   = _configCamera.orgY();
  region.p2   = _configCamera.orgY() + _configCamera.height() - 1;
  region.pbin = _configCamera.binY();
}

void PrincetonServer::checkTemperature()
{
  const int16 iCoolingTemp = _configCamera.coolingTemp();
  int16 iTemperatureCurrent = -1;  
  
  lockPlFunc();
  PICAM::getAnyParam(_hCam, PARAM_TEMP, &iTemperatureCurrent );
  releaseLockPlFunc();
    
  if ( iTemperatureCurrent >= iCoolingTemp + _iTemperatureTolerance ) 
  {
    printf( "PrincetonServer::runMonitorThread(): Chip temperature (%lf) is higher than the settings (%lf)\n", 
     iTemperatureCurrent/100.0, iCoolingTemp/100.0 );
    _iTemperatureStatus = 1;
  }
  else
    _iTemperatureStatus = 0;  
}

/*
 * Definition of private static consts
 */
const int       PrincetonServer::_iMaxCoolingTime;     
const int       PrincetonServer::_iTemperatureTolerance;
const int       PrincetonServer::_iFrameHeaderSize          = sizeof(CDatagram) + sizeof(Xtc) + sizeof(Princeton::FrameV1);
const int       PrincetonServer::_iCircBufFrameCount;
const int       PrincetonServer::_iMaxReadoutTime;
const timespec  PrincetonServer::_tmLockTimeout = { 0, 5000000 }; // 5 milliseconds 

void* PrincetonServer::threadEntryMonitor(void * pServer)
{
  ((PrincetonServer*) pServer)->runMonitorThread();
  return NULL;  
}

void* PrincetonServer::threadEntryCapture(void * pServer)
{
  ((PrincetonServer*) pServer)->runCaptureThread();
  return NULL;
}

/*
 * !!For debug only
 */
void PrincetonServer::lockPlFunc()
{
  if ( pthread_mutex_timedlock(&_mutexPlFuncs, &_tmLockTimeout) )
    printf( "PrincetonServer::lockPlFunc(): pthread_mutex_timedlock() failed\n" );    
}
void PrincetonServer::releaseLockPlFunc()
{
  pthread_mutex_unlock(&_mutexPlFuncs);
}

/*
 * Definition of private static data
 */
pthread_mutex_t PrincetonServer::_mutexPlFuncs = PTHREAD_MUTEX_INITIALIZER;    

} //namespace Pds 
