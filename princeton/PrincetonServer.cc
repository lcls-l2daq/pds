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
 _hCam(-1), _iCurShotIdStart(-1), _iCurShotIdEnd(-1), _fReadoutTime(0), _configCamera(), _pCircBufferWithHeader(NULL), 
 _iCameraAbortAndReset(0), _iEventCaptureEnd(0), _bForceCameraReset(false), _iTemperatureStatus(0), 
 _dgEvent(TypeId(), Src()), _pDgOut(NULL)
{       
  if ( checkInitSettings() != 0 )    
    throw PrincetonServerException( "PrincetonServer::PrincetonServer(): Invalid initial settings" );
      
  //if ( initCamera() != 0 )
  //  throw PrincetonServerException( "PrincetonServer::PrincetonServer(): initPrincetonCamera() failed" );    
  
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
  iFail= validateCameraSettings(config);  
  if ( iFail != 0 ) return 1;
  
  _configCamera = config;
  FrameV1::initStatic( _configCamera ); // associate frame objects with this camera config
    
  iFail = setupCooling();  
  if ( iFail != 0 ) return 2;  
  
  iFail = startContinuousCapture();
  if ( iFail != 0 ) return 3; 
  
  return 0;
}

int PrincetonServer::unconfigCamera()
{
  /* Stop the acquisition */
  if (!pl_exp_stop_cont(_hCam, CCS_HALT))
    printPvError("ImageCapture::testImageCaptureContinous():pl_exp_stop_cont() failed");

  /* Uninit the sequence */
  if (!pl_exp_uninit_seq()) 
    printPvError("ImageCapture::testImageCaptureContinous():pl_exp_uninit_seq() failed");
  
  free(_pCircBufferWithHeader);
  _pCircBufferWithHeader = NULL;
  
  return 0;
}

int PrincetonServer::startContinuousCapture()
{ 
  rgn_type region;  
  setupROI(region);
  PICAM::printROI(1, &region);

  if (!pl_exp_init_seq())
  {
    printPvError("PrincetonServer::startContinuousCapture(): pl_exp_init_seq() failed!\n");
    return 1; 
  }
 
  uns32 uFrameSize = 0;  
  const int16 iExposureMode = _configCamera.exposureMode();
  
  /*
   * _configCamera.exposureTime() time is measured in seconds,
   *  while iExposureTime for pl_exp_setup_cont() is measured in milliseconds
   *
   * Note: this value will be ignored by pl_exp_setup_cont(), if exposure mode = BULB_MODE
   */
  const uns32 iExposureTime = (int) ( _configCamera.exposureTime() * 1000 ); 
  
  rs_bool bStatus = 
   pl_exp_setup_cont(_hCam, 1, &region, iExposureMode, iExposureTime, &uFrameSize, CIRC_NO_OVERWRITE);
  if (!bStatus)
  {
    printPvError("PrincetonServer::startContinuousCapture(): pl_exp_setup_cont() failed!\n");
    return 2;
  }
  printf( "Frame size for continous capture = %lu\n", uFrameSize );  

  /* Set up a circular buffer */
  int iCircBufHeaderSize =  sizeof(CDatagram) + sizeof(FrameV1);
  if ( (int) uFrameSize < iCircBufHeaderSize )
  {
    printf("PrincetonServer::startContinuousCapture(): Frame(ROI) size (%lu bytes) is too small to fit in a DAQ packet header (%d bytes)\n",
     uFrameSize, iCircBufHeaderSize );
    return 3;
  }
  
  int iBufferSize = uFrameSize * _iCircBufFrameCount;
  _pCircBufferWithHeader = (unsigned char *) calloc(iCircBufHeaderSize + iBufferSize, 1);
  if (!_pCircBufferWithHeader)
  {
    printf("PrincetonServer::startContinuousCapture(): Memory allocation error!\n");
    return 4;
  }
  
  uns16 *pFrameBuffer = (uns16*) (_pCircBufferWithHeader + iCircBufHeaderSize);
  printf( "Continuous Frame Buffer starting from %p...\n", pFrameBuffer );

  /* Start the acquisition */
  bStatus = 
   pl_exp_start_cont(_hCam, pFrameBuffer, iBufferSize);
  if (!bStatus)
  {
    printPvError("PrincetonServer::startContinuousCapture():pl_exp_start_cont() failed");
    free(_pCircBufferWithHeader);
    return 5;
  }
 
  return 0;
}

int PrincetonServer::validateCameraSettings(Princeton::ConfigV1& config)
{
  if ( config.orgX() + config.width() > Princeton::ConfigV1::uPI_MTE_2048_Width 
    || config.orgY() + config.height() > Princeton::ConfigV1::uPI_MTE_2048_Height
    || config.binX() >= Princeton::ConfigV1::uPI_MTE_2048_Width / 32 
    || config.binY() >= Princeton::ConfigV1::uPI_MTE_2048_Height / 32 )
  {
    printf( "PrincetonServer::validateCameraSettings(): Config parameters are invalid\n" );
    return 1;
  }
  
  using PICAM::displayParamIdInfo;
  displayParamIdInfo(_hCam, PARAM_EXPOSURE_MODE,     "Exposure Mode");
  displayParamIdInfo(_hCam, PARAM_CLEAR_MODE,        "Clear Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_MODE,    "Shutter Open Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_DELAY,   "Shutter Open Delay");
  displayParamIdInfo(_hCam, PARAM_SHTR_CLOSE_DELAY,  "Shutter Close Delay");
  displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER,      "Edge Trigger" );
    
  displayParamIdInfo(_hCam, PARAM_EXP_RES,           "Exposure Resolution");
  displayParamIdInfo(_hCam, PARAM_EXP_RES_INDEX,     "Exposure Resolution Index");
  
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
    
    if ( _iCameraAbortAndReset )
    {
      _iCurShotIdEnd = -1;  
      _pDgOut = NULL;      
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

int PrincetonServer::captureStart(int iShotIdStart)
{
  _iCurShotIdStart = _iCurShotIdEnd = -1;

  if ( _iCameraAbortAndReset )
  {
    _iCurShotIdEnd = -1;    
    return 1;   
  }
    
  if ( _iEventCaptureEnd == 1 || _pDgOut != NULL )
  {
    printf( "PrincetonServer::captureStart(): Last frame has not been processed completely, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );     
    // Don't reset the shot id, because the capture thread should be processing the data
    return 2;
  }
  
  if ( _iCurShotIdStart >= 0 || _iCurShotIdEnd >= 0 )
  {
    printf( "PrincetonServer::captureStart(): Shot Id (Start or End) has not been reset since the last shot, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );    

    // Don't reset the shot id, if the capture thread might be processing the data; otherwise always reset the shot ids     
    if (!_bUseCaptureThread)
      _iCurShotIdStart = _iCurShotIdEnd = -1;    
    return 3;
  }    
    
  _iCurShotIdStart = iShotIdStart;  
    
  return 0;
}

int PrincetonServer::captureEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out)
{
  out = in; // default: return empty stream
  
  if ( _iCameraAbortAndReset != 0 || _iCurShotIdStart == -2 )
  {
    printf( "PrincetonServer::captureEnd(): Camera resetting. No data is outputted\n" );   
    _iCurShotIdEnd = -1;
    return 1;
  }
  
  if ( _iCurShotIdStart == -1 )
  {
    printf( "PrincetonServer::captureEnd(): No capture start event was received before the capture stop event\n" );
    _iCurShotIdEnd = -1;
    return 2;
  }  

  if ( _iEventCaptureEnd == 1 || _pDgOut != NULL )
  {
    printf( "PrincetonServer::captureEnd(): Last frame has not been processed completely, possibly"
     "because the events are coming too fast, or the capture thread has problems.\n" );     
    // Don't reset the shot id, because the capture thread should be processing the data, or the data is waiting to be read out
    return 3;
  }
  
  if ( _iCurShotIdEnd >= 0 )
  {
    printf( "PrincetonServer::captureEnd(): Shot Id (End) has not been reset since the last shot, possibly"
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
    return 4;
  }
      
  _iCurShotIdEnd = iShotIdEnd;
  if ( processFrame( in, out ) != 0 )
  {
    _iCurShotIdStart = _iCurShotIdEnd = -1;
    return 5;
  }

  /* 
   * Reset the shot-by-shot data
   */
  _iCurShotIdStart = _iCurShotIdEnd = -1;
    
  if ( _iTemperatureStatus != 0 )  
    return 6;    
      
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
  
  if ( _iCameraAbortAndReset != 0 || _iCurShotIdStart == -2 )
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
  _iCurShotIdStart = -2; // invalidate the shot id
  
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
    
  iFail = startContinuousCapture();
  if ( iFail != 0 )
    abort();
    
  printf( "PrincetonServer::abortAndResetCamera(): Camera reset ends.\n" );
  
  _iCameraAbortAndReset = 0;
}

int PrincetonServer::isExposureInProgress()
{  
  int16 iStatus = 0;
  uns32 uNumBytesTransfered;  
  uns32 uNumBufferAvailable;
  rs_bool bCheckOk;
  
  lockPlFunc();
  bCheckOk = 
   pl_exp_check_cont_status(_hCam, &iStatus, &uNumBytesTransfered, &uNumBufferAvailable);
  releaseLockPlFunc();  
  
  if (!bCheckOk)   
  {
    printf("PrincetonServer::isExposureInProgress(): pl_exp_check_cont_status() failed\n");
    return 0;
  }
  
  if (iStatus == READOUT_FAILED)
  {
    printf("PrincetonServer::isExposureInProgress(): pl_exp_check_cont_status() return status=READOUT_FAILED\n");
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
  uns32 uNumBufferAvailable = 0; // default value: 0
  
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
     pl_exp_check_cont_status(_hCam, &iStatus, &uNumBytesTransfered, &uNumBufferAvailable);
    releaseLockPlFunc();  
    
    if (!bCheckOk)
    {
      printPvError("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_cont_status() failed\n");
      return 1;
    }
    
    /*
     * READOUT_COMPLETE=FRAME_AVAILABLE, according to PVCAM 2.7 library manual
     */
    if (iStatus == READOUT_COMPLETE) // New frame available
      break;

    if (iStatus == READOUT_FAILED)
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_cont_status() return status=READOUT_FAILED\n");      
      return 2;
    }

    if (iStatus == READOUT_NOT_ACTIVE) // idle state: camera controller won't transfer any data
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_cont_status() return status=READOUT_NOT_ACTIVE\n");      
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

  /*
   * Discard excess frames
   *
   * If there are n (n>1) frame available, discard the older (n-1) frames and only retain the latest one  
   */  
  if ( uNumBufferAvailable > 1 )
  {
    printf("PrincetonServer::waitForNewFrameAvailable(): %lu frames has been captured but not processed yet.\n", uNumBufferAvailable );    
    discardExcessFrames(uNumBufferAvailable);
  }
  
  return 0;
}

void PrincetonServer::discardExcessFrames(uns32 uNumBufferAvailable)
{
  lockPlFunc();          
  for ( int i = 0; i< (int)uNumBufferAvailable - 1; i++ )
  {
    void* pFrameOld;      
    if (!pl_exp_get_oldest_frame(_hCam, &pFrameOld))
      printPvError("PrincetonServer::processFrame(): pl_exp_get_oldest_frame() failed");

    if ( !pl_exp_unlock_oldest_frame(_hCam) ) 
      printPvError("PrincetonServer::processFrame(): pl_exp_unlock_oldest_frame() failed");
  }
  releaseLockPlFunc();    
}

int PrincetonServer::processFrame(InDatagram* in, InDatagram*& out)
{  
  // default to return the empty (original) datagram without data
  out = in; 
  
  // Lock the current frame
  unsigned char* pFrameCurrent;
  rs_bool bOk;  
  lockPlFunc();      
  bOk = 
   pl_exp_get_oldest_frame(_hCam, (void**) &pFrameCurrent);
  releaseLockPlFunc();    
  if (!bOk)
  {
    printPvError("PrincetonServer::processFrame():pl_exp_get_oldest_frame() failed (for currentFrame)");
    return 2;
  }
  
  /*
   * Set frame object
   */  
  unsigned char* pFrameHeader = pFrameCurrent - sizeof(FrameV1);
  unsigned char* pXtcHeader   = pFrameHeader  - sizeof(Xtc);
  unsigned char* pCDatagram   = pXtcHeader    - sizeof(CDatagram);
        
  new (pFrameHeader) FrameV1(_iCurShotIdStart, _iCurShotIdEnd, _fReadoutTime);

  TypeId typePrincetonFrame(TypeId::Id_PrincetonFrame, FrameV1::Version);
  Xtc* pXtcFrame = 
   new ((char*)pXtcHeader) Xtc(typePrincetonFrame, _src);
  pXtcFrame->alloc( FrameV1::size() );
    
  out = 
   new (pCDatagram) CDatagram( in->datagram() ); 
  out->datagram().xtc.alloc( sizeof(Xtc) + FrameV1::size() );  
  
  // Release the frame lock
  lockPlFunc();
  bOk = 
   pl_exp_unlock_oldest_frame(_hCam);
  releaseLockPlFunc();        
  if (!bOk)
  {
    printPvError("PrincetonServer::processFrame(): pl_exp_unlock_oldest_frame() failed (for currentFrame)");
    return 3;
  }
  
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
    printf( "PrincetonServer::captureEnd(): File mode is not supported.\n" );
    return 1;
  }
  
  if ( !_bStreamMode && !_bUseCaptureThread )  
  {
    printf( "PrincetonServer::captureEnd(): Unsupported mode: File mode without capture thread.\n" );
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
 * Definition of private static data
 */
pthread_mutex_t PrincetonServer::_mutexPlFuncs = PTHREAD_MUTEX_INITIALIZER;    

} //namespace Pds 
