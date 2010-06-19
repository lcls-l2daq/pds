#include "PrincetonServer.hh" 
#include "PrincetonUtils.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/config/EvrConfigType.hh"

#include <errno.h>
#include <pthread.h> 
 
using std::string;

using PICAM::printPvError;

namespace Pds 
{

PrincetonServer::PrincetonServer(int iCamera, bool bDelayMode, bool bInitTest, const Src& src, int iDebugLevel) :
 _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), _src(src), _iDebugLevel(iDebugLevel),
 _hCam(-1), _bCameraInited(false), _bCaptureInited(false),
 _fPrevReadoutTime(0), _bSequenceError(false), _clockPrevDatagram(0,0), _iNumL1Event(0),
 _configCamera(), 
 _iCurShotId(-1), _fReadoutTime(0),  
 _poolFrameData(_iMaxFrameDataSize, _iPoolDataCount), _pDgOut(NULL),
 _CaptureState(CAPTURE_STATE_IDLE), _pTaskCapture(NULL), _routineCapture(*this)
{        
  if ( initCamera() != 0 )
    throw PrincetonServerException( "PrincetonServer::PrincetonServer(): initPrincetonCamera() failed" );    

  /*
   * Known issue:
   *
   * If initCaptureTask() is put here, occasionally we will miss some FRAME_COMPLETE event when we do the polling
   * Possible reason is that this function (constructor) is called by a different thread other than the polling thread.
   */    
}

PrincetonServer::~PrincetonServer()
{ 
  if ( _pTaskCapture != NULL )
    _pTaskCapture->destroy(); // task object will destroy the thread and release the object memory by itself

  /*
   * Wait for capture thread (if exists) to terminate
   */ 
  while ( _CaptureState == CAPTURE_STATE_RUN_TASK )
  {
    static int        iTotalWaitTime  = 0;
    static const int  iSleepTime      = 10000; // 10 ms
    
    timeval timeSleepMicro = {0, 10000}; 
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);    
    
    iTotalWaitTime += iSleepTime / 1000;
    if ( iTotalWaitTime >= _iMaxThreadEndTime )
    {
      printf( "PrincetonServer::~PrincetonServer(): timeout for waiting thread terminating\n" );
      break;
    }
  }
    
  deinitCamera();  
}

int PrincetonServer::initCamera()
{
  if ( _bCameraInited )
    return 0;

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );
    
  /* Initialize the PVCam Library */
  if (!pl_pvcam_init())
  {
    printPvError("PrincetonServer::initCamera(): pl_pvcam_init() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  }

  char strCamera[CAM_NAME_LEN];  /* camera name                    */
  
  /* Assume each machine only control one camera */
  if (!pl_cam_get_name(_iCamera, strCamera))
  {
    printPvError("PrincetonServer::initCamera(): pl_cam_get_name() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  }
  
  printf( "Opening camera [%d] name %s...\n", _iCamera, strCamera );
  
  if (!pl_cam_open(strCamera, &_hCam, OPEN_EXCLUSIVE))
  {
    printPvError("PrincetonServer::initCamera(): pl_cam_open() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  }
    
  timespec timeVal1;  
  clock_gettime( CLOCK_REALTIME, &timeVal1 );
  
  double fOpenTime = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;    
  printf("Camera Open Time = %6.3lf ms\n", fOpenTime);    

  int iCcdWidth = 0, iCcdHeight = 0;
  PICAM::getAnyParam(_hCam, PARAM_SER_SIZE, &iCcdWidth );
  PICAM::getAnyParam(_hCam, PARAM_PAR_SIZE, &iCcdHeight );
  printf( "CCD Width %d Height %d\n", iCcdWidth, iCcdHeight );
    
  if (_bInitTest)
  {
    if (initTest() != 0)
      return ERROR_PVCAM_FUNC_FAIL;
  }

  /* Initialize capture related functions */    
  if (!pl_exp_init_seq())
  {
    printPvError("PrincetonServer::initCamera(): pl_exp_init_seq() failed!\n");
    return ERROR_PVCAM_FUNC_FAIL; 
  }    
  
  printf( "Princeton Camera [%d] %s has been initialized\n", _iCamera, strCamera );
  
  _bCameraInited = true;    
  return 0;
}

int PrincetonServer::deinitCamera()
{  
  // If the camera has been init-ed before, and not deinit-ed yet  
  if ( _bCaptureInited )
    deinitCapture(); // deinit the camera explicitly    
  
  _bCameraInited = false;
  
  /* Uninit capture related functions */  
  if (!pl_exp_uninit_seq() ) 
  {
    printPvError("PrincetonServer::deinitCamera():pl_exp_uninit_seq() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  }
    
  if (!pl_cam_close(_hCam))
    printPvError("PrincetonServer::deinitCamera(): pl_cam_close() failed"); // Don't return here; continue to uninit the library

  if (!pl_pvcam_uninit())
  {
    printPvError("PrincetonServer::deinitCamera(): pl_pvcam_uninit() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  }
    
  printf( "Princeton Camera [%d] has been deinitialized\n", _iCamera );
  
  return 0;
}

int PrincetonServer::mapCamera()
{
  /*
   * Thread Issue:
   *
   * initCaptureTask() need to be put here. See the comment in the constructor function.
   */      
  if ( initCaptureTask() != 0 )
    return ERROR_SERVER_INIT_FAIL;
  
  return 0;
}

int PrincetonServer::configCamera(Princeton::ConfigV1& config)
{  
  //if ( initCamera() != 0 ) 
  //  return ERROR_SERVER_INIT_FAIL;
    
  if ( initCameraSettings(config) != 0 ) 
    return ERROR_SERVER_INIT_FAIL;
  
  _configCamera = config;
    
  if ( setupCooling() != 0 )
    return ERROR_SERVER_INIT_FAIL;  
  
  // Note: initCapture() has been moved to beginRun event
  //if ( initCapture() != 0 )
  //  return ERROR_SERVER_INIT_FAIL; 
    
  /*
   * Record the delay mode parameter in the config data
   *
   * Note: The delay mode was selected from the command line
   */
  config.setDelayMode( _bDelayMode?1:0 );
    
  // Note: initCapture() has been moved to map event
  //if ( initCaptureTask() != 0 )
  //  return ERROR_SERVER_INIT_FAIL;
  
  return 0;
}

int PrincetonServer::unconfigCamera()
{
  return 0;
}

int PrincetonServer::beginRunCamera()
{
  return initCapture();
}

int PrincetonServer::endRunCamera()
{
  /*
   * Check buffer control buffer status
   */  
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "PrincetonServer::endRunCamera(): Memory usage issue. Empty Data Pool is not totally free.\n" );

  return deinitCapture();
}

int PrincetonServer::initCapture()
{ 
  if ( _bCaptureInited )
    deinitCapture();
    
  LockCameraData lockInitCapture("PrincetonServer::initCapture()");    
    
  rgn_type region;  
  setupROI(region);
  PICAM::printROI(1, &region);
 
  const int16 iExposureMode = 0; // set exposure mode to TIMED_MODE, avoid using external TTL trigger
  
  /*
   * _configCamera.exposureTime() time is measured in seconds,
   *  while iExposureTime for pl_exp_setup() is measured in milliseconds
   */
  const uns32 iExposureTime = (int) ( _configCamera.exposureTime() * 1000 ); 

  uns32 uFrameSize;
  rs_bool bStatus = 
   pl_exp_setup_seq(_hCam, 1, 1, &region, iExposureMode, iExposureTime, &uFrameSize);
  if (!bStatus)
  {
    printPvError("PrincetonServer::initCapture(): pl_exp_setup_seq() failed!\n");
    return ERROR_PVCAM_FUNC_FAIL;
  }
    
  if ( (int)uFrameSize + _iFrameHeaderSize > _iMaxFrameDataSize )
  {
    printf( "PrincetonServer::initCapture():Frame size (%lu) + Frame header size (%d) "
     "is larger than internal data frame buffer size (%d)\n",
     uFrameSize, _iFrameHeaderSize, _iMaxFrameDataSize );
    return ERROR_INVALID_CONFIG;    
  }
     
  _bCaptureInited = true;
  
  printf( "Capture initialized\n" );

  if ( _iDebugLevel >= 2 )
    printf( "Frame size for image capture = %lu, exposure mode = %d, time = %lu\n",
     uFrameSize, iExposureMode, iExposureTime);  
  
  return 0;
}

int PrincetonServer::deinitCapture()
{
  if ( !_bCaptureInited )
    return 0;

  LockCameraData lockDeinitCapture("PrincetonServer::deinitCapture()");

  _bCaptureInited = false;
  
  resetFrameData(true);
      
  /* Stop the acquisition */
  rs_bool bStatus = 
   pl_exp_abort(_hCam, CCS_HALT);
  
  if (!bStatus)
  {
    printPvError("PrincetonServer::deinitCapture():pl_exp_abort() failed");
    return ERROR_PVCAM_FUNC_FAIL;
  } 
    
  /*
   * Reset the per-run data
   */
  _fPrevReadoutTime = 0;
  _bSequenceError   = false;
  _iNumL1Event      = 0;
  
  printf( "Capture deinitialized\n" );
  
  return 0;
}

int PrincetonServer::startCapture()
{
  if ( _pDgOut == NULL )
  {
    printf( "PrincetonServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }
    
  /* Start the acquisition */
  rs_bool bStatus = 
   pl_exp_start_seq(_hCam, (unsigned char*) _pDgOut + _iFrameHeaderSize );
  if (!bStatus)
  {
    printPvError("PrincetonServer::startCapture():pl_exp_start_seq() failed");    
    return ERROR_PVCAM_FUNC_FAIL;
  }  
  
  return 0;
}

int PrincetonServer::initCameraSettings(Princeton::ConfigV1& config)
{ 
  using PICAM::setAnyParam;
  using PICAM::displayParamIdInfo;
  
  int16 iSpeedTableIndex = config.readoutSpeedIndex();
  setAnyParam(_hCam, PARAM_SPDTAB_INDEX, &iSpeedTableIndex );     
  displayParamIdInfo(_hCam, PARAM_SPDTAB_INDEX, "Speed Table Index" );
  
  //displayParamIdInfo(_hCam, PARAM_EXPOSURE_MODE,    "Exposure Mode");
  //displayParamIdInfo(_hCam, PARAM_CLEAR_MODE,       "Clear Mode");
  //displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_MODE,   "Shutter Open Mode");
  //displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_DELAY,  "Shutter Open Delay");
  //displayParamIdInfo(_hCam, PARAM_SHTR_CLOSE_DELAY, "Shutter Close Delay");
  //  
  //displayParamIdInfo(_hCam, PARAM_EXP_RES,          "Exposure Resolution");
  //displayParamIdInfo(_hCam, PARAM_EXP_RES_INDEX,    "Exposure Resolution Index");
  
  //uns32 uTriggerEdge = EDGE_TRIG_POS;
  //PICAM::setAnyParam(_hCam, PARAM_EDGE_TRIGGER, &uTriggerEdge );    
  //displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER,     "Edge Trigger" );
  
  return 0;
}

int PrincetonServer::initTest()
{
  printf( "Running init test...\n" );
  
  rgn_type region;
  region.s1   = 0;
  region.s2   = 127;
  region.sbin = 1;
  region.p1   = 0;
  region.p2   = 127;
  region.pbin = 1;
  
  /* Init a sequence set the region, exposure mode and exposure time */
  pl_exp_init_seq();
  
  uns32 uFrameSize = 0;
  pl_exp_setup_seq(_hCam, 1, 1, &region, TIMED_MODE, 1, &uFrameSize);
  uns16* pFrameBuffer = (uns16 *) malloc(uFrameSize);

  timeval timeSleepMicroOrg = {0, 1000 }; // 1 milliseconds  
        
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );    
  
  pl_exp_start_seq(_hCam, pFrameBuffer);

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  
  uns32 uNumBytesTransfered;
  int iNumLoop = 0;
  int16 status = 0;

  /* wait for data or error */
  while (pl_exp_check_status(_hCam, &status, &uNumBytesTransfered) &&
         (status != READOUT_COMPLETE && status != READOUT_FAILED))
  {
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);       
    iNumLoop++;      
  }
  
  /* Check Error Codes */
  if (status == READOUT_FAILED)
    printPvError("PrincetonServer::initTest():pl_exp_check_status() failed");    

  timespec timeVal3;
  clock_gettime( CLOCK_REALTIME, &timeVal3 );

  double fReadoutTime = -1;
  PICAM::getAnyParam(_hCam, PARAM_READOUT_TIME, &fReadoutTime);
  
  double fStartupTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;    
  double fPollingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;    
  double fSingleFrameTime = fStartupTime + fPollingTime;
  printf("  Capture Setup Time = %6.1lfms Total Time = %6.1lfms\n", 
    fStartupTime, fSingleFrameTime );        
    
  // pl_exp_finish_seq(_hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  free(pFrameBuffer);  
  
  /*Uninit the sequence */
  pl_exp_uninit_seq();
   
  return 0;
}

int PrincetonServer::setupCooling()
{
  using namespace PICAM;

  if ( _iDebugLevel >= 2 )
    // Display cooling settings  
    displayParamIdInfo(_hCam, PARAM_COOLING_MODE, "Cooling Mode");

  //displayParamIdInfo(_hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature *Org*");  


  const int16 iCoolingTemp = (int)( _configCamera.coolingTemp() * 100 );

  if ( iCoolingTemp == _iMaxCoolingTemp  )
  {
    printf( "Skip cooling, since the cooling temperature is set to max value (%.1f C)\n", iCoolingTemp/100.0f );
    return 0;
  }

  int16 iTemperatureCurrent = -1;  
  getAnyParam(_hCam, PARAM_TEMP, &iTemperatureCurrent );
  if ( iTemperatureCurrent <= iCoolingTemp )
  {
    printf( "Skip cooling, since the cuurent  temperature (%.1f C) is lower than the setting (%.1f C)\n",
	    iTemperatureCurrent/100.0f, iCoolingTemp/100.0f );
    return 0;
  }  

  displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature Before Cooling" );  

  setAnyParam(_hCam, PARAM_TEMP_SETPOINT, &iCoolingTemp );
  displayParamIdInfo(_hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature" );
  
  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 millisecond    
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );      
  
  int iNumLoop = 0;
  
  while (1)
  {  
    setAnyParam(_hCam, PARAM_TEMP_SETPOINT, &iCoolingTemp );
    getAnyParam(_hCam, PARAM_TEMP, &iTemperatureCurrent );
    if ( iTemperatureCurrent <= iCoolingTemp ) break;
    
    if ( (iNumLoop+1) % 1000 == 0 )
      displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature *Updating*" );
    
    if ( iNumLoop > _iMaxCoolingTime ) break;
    
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
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
    return ERROR_COOLING_FAILURE;
  }
  
  return 0;
}

int PrincetonServer::initCaptureTask()
{
  if ( _pTaskCapture != NULL )
    return 0;
    
  _pTaskCapture = new Task(TaskObject("PrincetonServer"));
    
  if ( ! _bDelayMode )
  { // Prompt mode doesn't need to initialize the capture task, because the task will be performed in the event handler
    return 0;
  }
    
  return 0;
}

int PrincetonServer::runCaptureTask()
{
  if ( _CaptureState != CAPTURE_STATE_RUN_TASK )
  {
    printf( "PrincetonServer::runCaptureTask(): _CaptureState = %d. Capture task is not initialized correctly\n", 
     _CaptureState );
    return ERROR_INCORRECT_USAGE;      
  }

  LockCameraData lockCaptureProcess("PrincetonServer::runCaptureTask(): Start data polling and processing" );
      
  /*
   * Check if current run is being reset or program is exiting
   */
  if ( !_bCaptureInited )
  {      
    resetFrameData(true);
    return 0;
  }
  
  /*
   * Check if the datagram and frame data have been properly set up
   *
   * Note: This condition should NOT happen normally. Here is a logical check
   * and will output warnings.
   */
  if ( _pDgOut == NULL || _iCurShotId < 0 )
  {
    printf( "PrincetonServer::runCaptureTask(): Datagram or frame data have not been properly set up before the capture task\n" );
    resetFrameData(true);
    return ERROR_INCORRECT_USAGE;
  }
  
  int iFail = 0;
  do 
  {    
    iFail = waitForNewFrameAvailable();          
    if ( iFail != 0 ) break;
    
    iFail = processFrame();       
  }
  while (false);
  
  if ( iFail != 0 )
  {
    resetFrameData(true);
    return ERROR_FUNCTION_FAILURE;
  }
  
  /*
   * Set the damage bit if 
   *   1. temperature status is not good
   *   2. sequence error happened in the current run
   */
  if ( checkTemperature() != 0 )
    _pDgOut->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    
  /* 
   * Reset the frame data, except for the output datagram
   */
  _iCurShotId       = -1;  
  _fReadoutTime     = 0;
  
  _CaptureState = CAPTURE_STATE_DATA_READY;
  return 0;
}

int PrincetonServer::onEventReadoutPrompt(int iShotId, InDatagram* in, InDatagram*& out)
{  
  /*
   * Default to return empty datagram
   */
  out = in;
    
  ++_iNumL1Event; // update event counter
  
  /*
   * Chkec input arguments
   */
  if ( iShotId < 0 || in == NULL )
  {
    printf( "PrincetonServer::onEventReadoutPrompt(): Illegal input: shot id = %d , in datagram = %p\n", iShotId, in );
    return ERROR_INVALID_ARGUMENTS;
  }
  
  /*
   * This function is only useful when _bDelayMode = false
   */
  if ( _bDelayMode ) 
    return ERROR_INCORRECT_USAGE;
    
  /*
   * Check for sequence error
   */
  if ( checkSequence( in->datagram() ) != 0 )
    return ERROR_SEQUENCE_ERROR;
         
  // Set frame data
  _iCurShotId  = iShotId;  

  int iFail = 0;
  do 
  {
    iFail = setupFrame( in, out );
    if ( iFail != 0 ) break;
    
    iFail = startCapture();
    if ( iFail != 0 ) break;
        
    iFail = waitForNewFrameAvailable();
    if ( iFail != 0 ) break;        

    iFail = processFrame();
  }
  while (false);

  if ( iFail != 0 )
  {    
    resetFrameData(true);
    return ERROR_FUNCTION_FAILURE;  
  }
  
  /*
   * Update the readout time reference
   */  
  _fPrevReadoutTime = _fReadoutTime;  
      
  /* 
   * Reset the frame data, without releasing the output data
   *
   * Note: _pDgOut will not be released, because the data need to be sent out for use.
   */   
  resetFrameData(false);
      
  /*
   * Check temperature
   */  
  if ( checkTemperature() != 0 )  
    return ERROR_TEMPERATURE_HIGH; // Will cause the caller function to set the damage of the out datagram     
    
  /*
   * Check if sequence error happened and has not been reset
   */  
  if ( _bSequenceError )
    return ERROR_SEQUENCE_ERROR;
      
  return 0;
}

int PrincetonServer::onEventReadoutDelay(int iShotId, InDatagram* in)
{
  ++_iNumL1Event; // update event counter
  
  /*
   * Chkec input arguments
   */
  if ( iShotId < 0 || in == NULL )
  {
    printf( "PrincetonServer::onEventReadoutDelay(): Illegal input, shot id = %d , in datagram = %p\n", iShotId, in );
    return ERROR_INVALID_ARGUMENTS;
  }

  /*
   * This function is only useful when _bDelayMode = true
   */
  if ( !_bDelayMode ) 
    return ERROR_INCORRECT_USAGE;      
      
  /*
   * Chkec if we are allowed to add a new catpure task
   */
  if ( _CaptureState != CAPTURE_STATE_IDLE  )
  {         
    
    /*
     * Chkec if the data is ready, but has not been transfered out
     *
     * Note: This should NOT happen normally, unless the L1Accept handler didn't get 
     * the data out when the previous data is ready. Here is a logical check
     * and will output warnings.
     */
    if ( _CaptureState == CAPTURE_STATE_DATA_READY )
    {
      printf( "PrincetonServer::onEventReadoutDelay(): Previous image data has not been sent out\n" );
      resetFrameData(true);
      return ERROR_INCORRECT_USAGE;
    }
   
    /*
     * Remaning case:  _CaptureState == CAPTURE_STATE_RUN_TASK
     * capture thread is still busy polling or processing the previous image data
     *
     * Note: Originally, this is a possible case for software adaptive mode, where the 
     * L1Accept event are raised no matter whether the polling thread is busy or not. 
     *
     * However, since current EVR implementation doesn't support software adaptive mode, 
     * this case is NOT a normal case.
     */    
    
    printf( "PrincetonServer::onEventReadoutDelay(): Capture task is running. It is impossible to start a new capture.\n" );
    
    /*
     * Here we don't reset the frame data, because the capture task is running and will use the data later
     */
    return ERROR_INCORRECT_USAGE; // No error for adaptive mode
  }
  
  _iCurShotId     = iShotId;  
  InDatagram* out = NULL;
  
  int iFail = 0;
  do 
  {
    iFail = setupFrame( in, out );
    if ( iFail != 0 ) break;

    iFail = startCapture();
  }
  while (false);
    
  if ( iFail != 0 )
  {
    resetFrameData(true);
    return ERROR_FUNCTION_FAILURE;
  }    
    
  _iCurShotId   = iShotId;
  _CaptureState = CAPTURE_STATE_RUN_TASK;  // Notify the capture thread to start data polling/processing
  _pTaskCapture->call( &_routineCapture );
  return 0;
}

int PrincetonServer::getDelayData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream

  if ( _CaptureState != CAPTURE_STATE_DATA_READY )
    return 0;
  
  /*
   * This function is only useful when _bDelayMode = true
   */
  if ( !_bDelayMode ) 
    return ERROR_INCORRECT_USAGE;      
    
  /*
   * Check if the datagram and frame data have been properly set up
   *
   * Note: This condition should NOT happen normally. Here is a logical check
   * and will output warnings.
   */
  if ( _pDgOut == NULL )
  {
    printf( "PrincetonServer::getDelayData(): Datagram is not properly set up\n" );
    resetFrameData(true);
    return ERROR_LOGICAL_FAILURE;      
  }
    
    
  Datagram& dgIn  = in->datagram();
  Datagram& dgOut = _pDgOut->datagram();
  
  /*
   * Backup the orignal Xtc data
   *
   * Note that it is not correct to use  Xtc xtc1 = xtc2, which means using the Xtc constructor,
   * instead of the copy operator to do the copy. In this case, the xtc data size will be set to 0.
   */
  Xtc xtcOutBkp; 
  xtcOutBkp = dgOut.xtc;

  /*
   * Compose the datagram
   *
   *   1. Use the header from dgIn 
   *   2. Use the xtc (data header) from dgOut
   *   3. Use the data from dgOut (the data was located right after the xtc)
   */
  dgOut     = dgIn;   
  dgOut.xtc = xtcOutBkp;
      
  out       = _pDgOut;    
  
  /* 
   * Reset the frame data, without releasing the output data
   *
   * Note: 
   *   1. _pDgOut will be set to NULL, so that the same data will never be sent out twice
   *   2. _pDgOut will not be released, because the data need to be sent out for use.
   */
  resetFrameData(false);  
  return 0;
}

int PrincetonServer::getLastDelayData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream
  
  /*
   * This function is only useful when _bDelayMode = true
   */
  if ( !_bDelayMode ) 
    return ERROR_INCORRECT_USAGE;  
  
  if ( _CaptureState == CAPTURE_STATE_IDLE )
    return 0;
  
  static timespec tsWaitStart;
  clock_gettime( CLOCK_REALTIME, &tsWaitStart );
  
  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds
  
  while (_CaptureState != CAPTURE_STATE_DATA_READY)
  {
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro); 
        
          
    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );
    
    int iWaitTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1000000 + 
     ( tsCurrent.tv_sec - tsWaitStart.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime >= _iMaxLastEventTime )
    {
      printf( "PrincetonServer::getLastDelayData(): Waiting time is too long. Skip the final data\n" );          
      return ERROR_FUNCTION_FAILURE;
    }
  } // while (1)
  
  
  return getDelayData(in, out);      
}

int PrincetonServer::checkReadoutEventCode(InDatagram* in)
{
  /*
   * The evrData comes from NetDgServer, and so it has two extra levels of Xtc for wrapping the EVR data
   */
  const Xtc& xtcEvrData = in->datagram().xtc;  
  if ( xtcEvrData.sizeofPayload() == 0 )
    return ERROR_FUNCTION_FAILURE;

  /*
   * Only support EvrData Version 3
   */
  const Xtc&          xtcData     = *(const Xtc*) (xtcEvrData.payload() + sizeof(Xtc));
  const TypeId&       typeIdData  = xtcData.contains;
  if ( typeIdData.id()      != TypeId::Id_EvrData && 
       typeIdData.version() != 3)
  {
    printf( "PrincetonServer::checkReadoutEventCode(): Evr data contains invalid type id %s, version %d\n", 
      TypeId::name(typeIdData.id()), typeIdData.version() );
    return ERROR_FUNCTION_FAILURE;
  }
  
  const EvrDataType&  evrData = *(const EvrDataType*) (xtcData.payload());
  
  if ( _iDebugLevel >= 3 )
    printf( "# of fifo events: %d\n", evrData.numFifoEvents() );
  
  bool bReadoutEventFound = false;
  for ( unsigned int iEventIndex=0; iEventIndex< evrData.numFifoEvents(); iEventIndex++ )
  {
    const EvrDataType::FIFOEvent& event = evrData.fifoEvent(iEventIndex);
    if ( event.EventCode == _configCamera.readoutEventCode() )
    {
      if ( _iDebugLevel < 3 )
        return 0;
        
      bReadoutEventFound = true;        
    }

    if ( _iDebugLevel >= 3 )    
      printf( "[%02u] Event Code %u  TimeStampHigh 0x%x  TimeStampLow 0x%x\n",
        iEventIndex, event.EventCode, event.TimestampHigh, event.TimestampLow );
  }
  
  return (bReadoutEventFound? 0: ERROR_FUNCTION_FAILURE);
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
    
    bCheckOk = 
     pl_exp_check_status(_hCam, &iStatus, &uNumBytesTransfered);
    
    if (!bCheckOk)
    {
      if ( pl_error_code() == 0 )
        return ERROR_PVCAM_FUNC_FAIL;
      
      printPvError("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() failed\n");
      return ERROR_PVCAM_FUNC_FAIL;
    }
    
    /*
     * READOUT_COMPLETE=FRAME_AVAILABLE, according to PVCAM 2.7 library manual
     */
    if (iStatus == READOUT_COMPLETE) // New frame available
      break;

    if (iStatus == READOUT_FAILED)
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() return status=READOUT_FAILED\n");      
      return ERROR_LOGICAL_FAILURE;
    }

    if (iStatus == READOUT_NOT_ACTIVE) // idle state: camera controller won't transfer any data
    {
      printf("PrincetonServer::waitForNewFrameAvailable(): pl_exp_check_status() return status=READOUT_NOT_ACTIVE\n");      
      return ERROR_FUNCTION_FAILURE;
    }
    
    /*
     * sleep and do the next iteration, for the cases:
     * 
     * READOUT_IN_PROGRESS, EXPOSURE_IN_PROGRESS, ACQUISITION_IN_PROGRESS
     */
     
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro); 
        
          
    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );
    
    int iWaitTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1000000 + 
     ( tsCurrent.tv_sec - tsWaitStart.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime >= _iMaxReadoutTime )
    {
      printf( "PrincetonServer::waitForNewFrameAvailable(): Readout time is longer than %d miliseconds. Capture is stopped\n",
       _iMaxReadoutTime );    
      return ERROR_FUNCTION_FAILURE;
    }     
  } // while (1)

  timespec tsWaitEnd;
  clock_gettime( CLOCK_REALTIME, &tsWaitEnd );
  
  _fReadoutTime = (tsWaitEnd.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsWaitEnd.tv_sec - tsWaitStart.tv_sec ); // in seconds
  
  if ( _iNumL1Event <= _iMaxEventReport )
    printf( "Readout time report [%d]: %f s\n", _iNumL1Event, _fReadoutTime );
    
  return 0;
}

int PrincetonServer::processFrame()
{   
  if ( _pDgOut == NULL )
  {
    printf( "PrincetonServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }
      
  /*
   * Set frame object
   */
  unsigned char*  pFrameHeader  = (unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc);  
  new (pFrameHeader) Princeton::FrameV1(_iCurShotId, _fReadoutTime);
      
  return 0;
}

int PrincetonServer::setupFrame(InDatagram* in, InDatagram*& out)
{ 
  if ( _poolFrameData.numberOfFreeObjects() <= 0 )
  {
    printf( "PrincetonServer::setupFrame(): Pool is full, and cannot provide buffer for new datagram\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  const int iFrameSize =_configCamera.frameSize();
  
  out = 
   new ( &_poolFrameData ) CDatagram( in->datagram() ); 
  out->datagram().xtc.alloc( sizeof(Xtc) + iFrameSize ); // !! debug

  /*
   * Set the output datagram pointer
   *
   * Note: This pointer will be used in processFrame(). In the case of delay mode,
   * processFrame() will be exectued in another thread.
   */
  _pDgOut = out;
  
  if ( _iDebugLevel >= 3 )
  {
    printf( "PrincetonServer::setupFrame(): pool free objects#: %d , data gram: %p\n", 
     _poolFrameData.numberOfFreeObjects(), _pDgOut  );
  }
  
  /*
   * Set frame object
   */    
  unsigned char* pXtcHeader = (unsigned char*) _pDgOut + sizeof(CDatagram);
     
  TypeId typePrincetonFrame(TypeId::Id_PrincetonFrame, Princeton::FrameV1::Version);
  Xtc* pXtcFrame = 
   new ((char*)pXtcHeader) Xtc(typePrincetonFrame, _src);
  pXtcFrame->alloc( iFrameSize );
      
  return 0;
}

int PrincetonServer::resetFrameData(bool bDelOutDatagram)
{
  /*
   * Reset per-frame data
   */
  _iCurShotId       = -1;
  _fReadoutTime     = 0;
    
  /*
   * Update Capture Thread Control and I/O variables
   */
  _CaptureState     = CAPTURE_STATE_IDLE;

  /*
   * Reset buffer data
   */
  if (bDelOutDatagram)  delete _pDgOut;
  _pDgOut = NULL;
  
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

int PrincetonServer::checkTemperature()
{
  const int16 iCoolingTemp = (int)( _configCamera.coolingTemp() * 100 );
  int16 iTemperatureCurrent = -1;  
  
  PICAM::getAnyParam(_hCam, PARAM_TEMP, &iTemperatureCurrent );

  if ( _iNumL1Event % 10 == 1 )
  {
    printf( "CCD Temperature report [%d]: %.1f C\n", _iNumL1Event, iTemperatureCurrent/100.f );
  }
    
  if ( iTemperatureCurrent >= iCoolingTemp + _iTemperatureTolerance ) 
  {
    printf( "** PrincetonServer::checkTemperature(): Chip temperature (%.1f) is higher than the settings (%.1f)\n", 
     iTemperatureCurrent/100.0f, iCoolingTemp/100.0f );
    return ERROR_TEMPERATURE_HIGH;
  }
  
  return 0;
}

int PrincetonServer::checkSequence( const Datagram& datagram )
{
  /*
   * Check for sequence error
   */
  const ClockTime clockCurDatagram  = datagram.seq.clock();
  
  /*
   * Note: ClockTime.seconds() and ClockTime.nanoseconds() are unsigned values, and
   *   the difference of two unsigned values will be also interpreted as an unsigned value.
   *   We need to convert the computed differences to signed values by using (int) operator.
   *   Otherwise, negative values will be interpreted as large positive values.
   */
  const float     fDeltaTime        = (int) ( clockCurDatagram.seconds() - _clockPrevDatagram.seconds() ) +
   (int) ( clockCurDatagram.nanoseconds() - _clockPrevDatagram.nanoseconds() ) * 1.0e-9f;
   
  if ( fDeltaTime < _fPrevReadoutTime * _fEventDeltaTimeFactor )
  {
    // Report the error for the first few L1 events
    if ( _iNumL1Event <= _iMaxEventReport )
      printf( "** PrincetonServer::checkSequence(): Sequence error. Event delta time (%fs) < Prev Readout Time (%fs) * Factor (%f)\n",
        fDeltaTime, _fPrevReadoutTime, _fEventDeltaTimeFactor );
        
    _bSequenceError = true;
    return ERROR_SEQUENCE_ERROR;
  }
  
  if ( _iDebugLevel >= 3 )
  {
    printf( "PrincetonServer::checkSequence(): Event delta time (%fs), Prev Readout Time (%fs), Factor (%f)\n",
      fDeltaTime, _fPrevReadoutTime, _fEventDeltaTimeFactor );
  }
  _clockPrevDatagram = clockCurDatagram;
  
  return 0;
}

/*
 * Definition of private static consts
 */
const int       PrincetonServer::_iMaxCoolingTime;     
const int       PrincetonServer::_iTemperatureTolerance;
const int       PrincetonServer::_iFrameHeaderSize      = sizeof(CDatagram) + sizeof(Xtc) + sizeof(Princeton::FrameV1);
const int       PrincetonServer::_iMaxFrameDataSize     = 2048*2048*2 + _iFrameHeaderSize;
const int       PrincetonServer::_iPoolDataCount;
const int       PrincetonServer::_iMaxReadoutTime;
const int       PrincetonServer::_iMaxThreadEndTime;
const int       PrincetonServer::_iMaxLastEventTime;
const int       PrincetonServer::_iMaxEventReport;
const float     PrincetonServer::_fEventDeltaTimeFactor = 1.1f;
/*
 * Definition of private static data
 */
pthread_mutex_t PrincetonServer::_mutexPlFuncs = PTHREAD_MUTEX_INITIALIZER;    


PrincetonServer::CaptureRoutine::CaptureRoutine(PrincetonServer& server) : _server(server)
{
}

void PrincetonServer::CaptureRoutine::routine(void)
{
  _server.runCaptureTask();
}

} //namespace Pds 
