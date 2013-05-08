#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "pds/config/FliDataType.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/CfgPath.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Table.hh"

#include "FliServer.hh"

using std::string;

namespace Pds
{

FliServer::FliServer(int iCamera, bool bDelayMode, bool bInitTest, const Src& src, string sConfigDb, int iSleepInt, int iDebugLevel) :
 _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), _src(src),
 _sConfigDb(sConfigDb), _iSleepInt(iSleepInt), _iDebugLevel(iDebugLevel),
 _hCam(-1), _bCameraInited(false), _bCaptureInited(false),
 _iDetectorWidth(-1), _iDetectorHeight(-1), _iDetectorOrgX(-1), _iDetectorOrgY(-1), _iDetectorDataWidth(-1), _iDetectorDataHeight(-1),
 _iImageWidth(-1), _iImageHeight(-1),
 _fPrevReadoutTime(0), _bSequenceError(false), _clockPrevDatagram(0,0), _iNumExposure(0),
 _config(),
 _fReadoutTime(0),
 _poolFrameData(_iMaxFrameDataSize, _iPoolDataCount), _pDgOut(NULL),
 _CaptureState(CAPTURE_STATE_IDLE), _pTaskCapture(NULL), _routineCapture(*this)
{
  if ( initDevice() != 0 )
    throw FliServerException( "FliServer::FliServer(): initFli() failed" );

  /*
   * Known issue:
   *
   * If initCaptureTask() is put here, occasionally we will miss some FRAME_COMPLETE event when we do the polling
   * Possible reason is that this function (constructor) is called by a different thread other than the polling thread.
   */
}

FliServer::~FliServer()
{
  /*
   * Note: This function may be called by the signal handler, and need to be finished quickly
   */
  if (_hCam == -1)
    return;

  int iError;
  iError = FLICancelExposure(_hCam);
  if (iError != 0)
    printf("FliServer::~FliServer(): FLICancelExposure() failed. Error code %d: %s\n", iError, strerror(-iError));

  deinit();

  if ( _pTaskCapture != NULL )
    _pTaskCapture->destroy(); // task object will destroy the thread and release the object memory by itself
}

int FliServer::initDevice()
{
  if ( _bCameraInited )
    return 0;

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  FLISetDebugLevel( NULL /* No output to kernel log */ , FLIDEBUG_ALL);

  const flidomain_t   domain = FLIDOMAIN_USB;
  string              strCamera;
  int                 iError;
  iError = getCamera(domain, _iCamera, strCamera);
  if (iError != 0)
    return 1;

  iError = FLIOpen( &_hCam, (char*) strCamera.c_str(), FLIDEVICE_CAMERA | domain);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIOpen() failed (hcam = %d). Error code %d: %s\n", (int) _hCam, iError, strerror(-iError));
    _hCam = -1;
    return 2;
  }

  FLISetDebugLevel( NULL /* No output to kernel log */ , (FLIDEBUG_WARN | FLIDEBUG_FAIL));

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  double fOpenTime = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  printf("Camera Open Time = %6.1lf ms\n", fOpenTime);

  return 0;
}

int FliServer::initSetup()
{
  if ( _bCameraInited )
    return 0;

  long int iDatX, iDatY, iDatW, iDatH;
  int iError = FLIGetArrayArea( _hCam, &iDatX, &iDatY, &iDatW, &iDatH);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIGetArrayArea() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 3;
  }
  iDatW -= iDatX;
  iDatH -= iDatY;

  long int iVisX, iVisY, iVisW, iVisH;
  iError = FLIGetVisibleArea( _hCam, &iVisX, &iVisY, &iVisW, &iVisH);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIGetVisibleArea() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }
  iVisW -= iVisX;
  iVisH -= iVisY;

  _iDetectorWidth  = iVisW;
  _iDetectorHeight = iVisH;
  _iDetectorOrgX   = iDatX - iVisX;
  _iDetectorOrgY   = iDatY - iVisY;
  _iDetectorDataWidth  = iDatW;
  _iDetectorDataHeight = iDatH;

  printf("Detector Raw Size (%d,%d) W %d H %d\n", _iDetectorOrgX, _iDetectorOrgY, _iDetectorDataWidth, _iDetectorDataHeight);

  double fTemperature;
  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }
  printf( "Detector Width %d Height %d Temperature %.1lf C\n", _iDetectorWidth, _iDetectorHeight, fTemperature);

  if (_bInitTest)
  {
    if (initTest() != 0)
      return ERROR_SDK_FUNC_FAIL;
  }

  int iFail = initCameraBeforeConfig();
  if (iFail != 0)
  {
    printf("FliServer::init(): initCameraBeforeConfig() failed!\n");
    return ERROR_FUNCTION_FAILURE;
  }

  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }
  printf( "Detector Width %d Height %d Temperature %.1lf C\n", _iDetectorWidth, _iDetectorHeight, fTemperature);

  const int iDevId = ((DetInfo&)_src).devId();
  printf( "Fli Camera [%d] (device %d) has been initialized\n", iDevId, _iCamera );
  _bCameraInited = true;

  return 0;
}

int FliServer::deinit()
{
  // If the camera has been init-ed before, and not deinit-ed yet
  if ( _bCaptureInited )
    deinitCapture(); // deinit the camera explicitly

  int iError = 0;
  if (_hCam != -1)
    iError = FLIClose(_hCam);
  if (iError != 0)
    printf("FliServer::deinit(): FLIClose() failed. Error code %d: %s\n", iError, strerror(-iError));

  _bCameraInited = false;

  const int iDevId = ((DetInfo&)_src).devId();
  printf( "Fli Camera [%d] (device %d) has been deinitialized\n", iDevId, _iCamera );
  return 0;
}

int FliServer::map()
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

int FliServer::config(FliConfigType& config, std::string& sConfigWarning)
{
  if ( configCamera(config, sConfigWarning) != 0 )
    return ERROR_SERVER_INIT_FAIL;

  const int iDevId = ((DetInfo&)_src).devId();

  if ( (int) config.width() > _iDetectorWidth || (int) config.height() > _iDetectorHeight)
  {
    char sMessage[128];
    sprintf( sMessage, "!!! FLI %d ConfigSize (%d,%d) > Det Size(%d,%d)\n", iDevId, config.width(), config.height(), _iDetectorWidth, _iDetectorHeight);
    printf(sMessage);
    sConfigWarning += sMessage;
    config.setWidth (_iDetectorWidth);
    config.setHeight(_iDetectorHeight);
  }

  if ( (int) (config.orgX() + config.width()) > _iDetectorWidth ||
       (int) (config.orgY() + config.height()) > _iDetectorHeight)
  {
    char sMessage[128];
    sprintf( sMessage, "!!! FLI %d ROI Boundary (%d,%d) > Det Size(%d,%d)\n", iDevId,
             config.orgX() + config.width(), config.orgY() + config.height(), _iDetectorWidth, _iDetectorHeight);
    printf(sMessage);
    sConfigWarning += sMessage;
    config.setWidth (_iDetectorWidth - config.orgX());
    config.setHeight(_iDetectorHeight - config.orgY());
  }

  //Note: We don't send error for cooling incomplete
  setupCooling( (double) _config.coolingTemp() );
  //if ( setupCooling() != 0 )
    //return ERROR_SERVER_INIT_FAIL;

  _config = config;

  int     iError;
  double  fTemperature;
  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::init(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }
  printf( "Detector Width %d Height %d Temperature %.1lf C\n", _iDetectorWidth, _iDetectorHeight, fTemperature);

  return 0;
}

int FliServer::unconfig()
{
  return 0;
}

int FliServer::beginRun()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "FliServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "BeginRun Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  /*
   * Reset the per-run data
   */
  _fPrevReadoutTime = 0;
  _bSequenceError   = false;
  _iNumExposure      = 0;

  return 0;
}

int FliServer::endRun()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "FliServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "EndRun Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  return 0;
}

int FliServer::beginCalibCycle()
{
  return 0;
}

int FliServer::endCalibCycle()
{
  return 0;
}

int FliServer::enable()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "FliServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "Enable Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );


  int iFail = initCapture();

  if ( iFail != 0 )
    return ERROR_FUNCTION_FAILURE;

  return 0;
}

int FliServer::disable()
{
  // Note: Here we don't worry if the data pool is not free, because the
  //       traffic shaping thead may still holding the L1 Data
  if ( _iDebugLevel >= 3 )
    printf( "Disable Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  int iFail  = deinitCapture();

  if ( iFail != 0 )
    return ERROR_FUNCTION_FAILURE;

  return 0;
}

int FliServer::initCapture()
{
  if ( _bCaptureInited )
    deinitCapture();

  LockCameraData lockInitCapture(const_cast<char*>("FliServer::initCapture()"));

  int iError = setupROI();
  if (iError != 0)
    return 1;

  /*
   * _config.exposureTime() time is measured in seconds,
   *  while iExposureTime for FLISetExposureTime() is measured in milliseconds
   */
  const long int iExposureTime = (int) ( _config.exposureTime() * 1000 );

  iError = FLISetExposureTime(_hCam, iExposureTime);
  if (iError != 0)
  {
    printf("FliServer::initCapture(): FLISetExposureTime() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 2;
  }

  iError = FLISetFrameType(_hCam, FLI_FRAME_TYPE_NORMAL);
  if (iError != 0)
  {
    printf("FliServer::initCapture(): FLISetFrameType() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 3;
  }

  if ( _config.frameSize() - (int) sizeof(FliDataType) + _iFrameHeaderSize > _iMaxFrameDataSize )
  {
    printf( "FliServer::initCapture():Frame size (%i) + Frame header size (%d)"
     "is larger than internal data frame buffer size (%d)\n",
     _config.frameSize() - (int) sizeof(FliDataType), _iFrameHeaderSize, _iMaxFrameDataSize );
    return 4;
  }

  _bCaptureInited = true;

  printf( "Capture initialized\n" );

  if ( _iDebugLevel >= 2 )
    printf( "Frame size for image capture = %i. Exposure time = %f s\n",
     _config.frameSize(), _config.exposureTime());

  return 0;
}

int FliServer::deinitCapture()
{
  if ( !_bCaptureInited )
    return 0;
  
  LockCameraData lockDeinitCapture(const_cast<char*>("FliServer::deinitCapture()"));

  _bCaptureInited = false;

  resetFrameData(true);


  int iError;
  iError = FLICancelExposure(_hCam);
  if (iError != 0)
  {
    printf("FliServer::deinitCapture(): FLICancelExposure() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  printf( "Capture deinitialized\n" );

  return 0;
}

int FliServer::startCapture()
{
  if ( _pDgOut == NULL )
  {
    printf( "FliServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  int iError;
  iError = FLIExposeFrame(_hCam);
  if (iError != 0)
  {
    printf("FliServer::startCapture(): FLIExposeFrame() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  return 0;
}

int FliServer::configCamera(FliConfigType& config, std::string& sConfigWarning)
{
  int   iError;
  char  sBuffer[64];
  iError = FLIGetLibVersion(sBuffer, sizeof(sBuffer));
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetLibVersion() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }
  printf("FLI library ver %s\n", sBuffer);

  iError = FLIGetModel(_hCam, sBuffer, sizeof(sBuffer));
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetModel() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 2;
  }

  long int iHwRevsion;
  iError = FLIGetHWRevision(_hCam, &iHwRevsion);
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetHWRevision() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 3;
  }

  long int iFwRevsion;
  iError = FLIGetFWRevision(_hCam, &iFwRevsion);
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetFWRevision() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }

  double fPixelSizeX, fPixelSizeY;
  iError = FLIGetPixelSize(_hCam, &fPixelSizeX, &fPixelSizeY);
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetPixelSize() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 5;
  }

  printf("Model: %s HW Rev: %ld FW Rev: %ld Pixel Size (um): x %lg y %lg\n", sBuffer,
    iHwRevsion, iFwRevsion, fPixelSizeX, fPixelSizeY);

  /*
   * set speed index
   *
   * speed index = 0 => set mode = 1 (1MHz)
   * speed index = 1 => set mode = 0 (8MHz)
   */
  flimode_t iModeSet = (config.readoutSpeedIndex() == 0 ? 1 : 0);
  iError = FLISetCameraMode(_hCam, iModeSet);
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLISetCameraMode() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 6;
  }
  flimode_t iModeGet = -1;
  iError = FLIGetCameraMode(_hCam, &iModeGet);
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetCameraMode() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 7;
  }
  char sCameraMode[32];
  iError = FLIGetCameraModeString(_hCam, iModeGet, sCameraMode, sizeof(sCameraMode) );
  if (iError != 0)
  {
    printf("FliServer::configCamera(): FLIGetCameraModeString() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 8;
  }
  printf("Speed : %d %s\n", (int) iModeGet, sCameraMode);
  if (iModeGet != iModeSet)
  {
    printf("FliServer::configCamera(): Camera Mode get value (%d) != set value (%d)\n", (int) iModeGet, (int) iModeSet);
    return 9;
  }

  // set gain ...

  return 0;
}

int FliServer::initCameraBeforeConfig()
{
  if (_sConfigDb.empty())
    return 0;

  size_t iIndexComma = _sConfigDb.find_first_of(',');

  string sConfigPath, sConfigType;
  if (iIndexComma == string::npos)
  {
    sConfigPath = _sConfigDb;
    sConfigType = "PRINCETON_BURST";
  }
  else
  {
    sConfigPath = _sConfigDb.substr(0, iIndexComma);
    sConfigType = _sConfigDb.substr(iIndexComma+1, string::npos);
  }

  // Setup ConfigDB and Run Key
  int runKey;
  { Pds_ConfigDb::Experiment expt((const Pds_ConfigDb::Path&)Pds_ConfigDb::Path(sConfigPath),
                                  Pds_ConfigDb::Experiment::NoLock);
    expt.read();
    const Pds_ConfigDb::TableEntry* entry = expt.table().get_top_entry(sConfigType);
    if (entry == NULL)
      {
        printf("FliServer::initCameraBeforeConfig(): Invalid config db path [%s] type [%s]\n",sConfigPath.c_str(), sConfigType.c_str());
        return 1;
      }
    runKey = strtoul(entry->key().c_str(),NULL,16);
  }

  const TypeId typeFliConfig = TypeId(TypeId::Id_FliConfig, FliConfigType::Version);

  char strConfigPath[128];
  sprintf(strConfigPath,"%s/keys/%s",sConfigPath.c_str(),CfgPath::path(runKey,_src,typeFliConfig).c_str());
  printf("Config Path: %s\n", strConfigPath);

  int fdConfig = open(strConfigPath, O_RDONLY);
  FliConfigType config;
  int iSizeRead = read(fdConfig, &config, sizeof(config));
  if (iSizeRead != sizeof(config))
  {
    printf("FliServer::initCameraBeforeConfig(): Read config data of incorrect size. Read size = %d (should be %d) bytes\n",
      iSizeRead, (int) sizeof(config));
    return 2;
  }

  printf("Setting cooling temperature: %f\n", config.coolingTemp());
  setupCooling( (double) config.coolingTemp() );

  return 0;
}

int FliServer::initTest()
{
  printf( "Running init test...\n" );

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  int iError;
  iError = FLISetImageArea( _hCam, 0, 0, 1024, 1024);
  if (iError != 0)
  {
    printf("FliServer::initTest(): FLISetImageArea() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  iError = FLISetHBin( _hCam, 1);
  if (iError != 0)
  {
    printf("FliServer::initTest(): FLISetHBin() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 2;
  }

  iError = FLISetVBin( _hCam, 1);
  if (iError != 0)
  {
    printf("FliServer::initTest(): FLISetVBin() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 3;
  }

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  iError = FLIExposeFrame(_hCam);
  if (iError != 0)
  {
    printf("FliServer::initTest(): FLIExposeFrame() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );

  while (true)
  {
    long int iTimeRemain = 0; // in ms
    iError = FLIGetExposureStatus(_hCam, &iTimeRemain);
    if (iError != 0)
    {
      printf("FliServer::initTest(): FLIGetExposureStatus() failed. Error code %d: %s\n", iError, strerror(-iError));
      break;
    }

    if (iTimeRemain == 0)
      break;

    timeval timeSleepMicro = {0, iTimeRemain*1000};
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
  }

  timespec timeVal3;
  clock_gettime( CLOCK_REALTIME, &timeVal3 );

  double fInitTime    = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  double fStartupTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  double fPollingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
  double fSingleFrameTime = fStartupTime + fPollingTime;
  printf("  Capture Init Time = %6.1lfms Setup Time = %6.1lfms Total Time = %6.1lfms\n",
    fInitTime, fStartupTime, fSingleFrameTime );

  return 0;
}

int FliServer::setupCooling(double fCoolingTemperature)
{
  int     iError;
  double  fTemperature;
  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::setupCooling(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  printf("Temperature Before cooling: %.1lf C\n", fTemperature);

  iError = FLISetTemperature( _hCam, fCoolingTemperature);
  if (iError != 0)
  {
    printf("FliServer::setupCooling(): FLISetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 2;
  }

  const static timeval timeSleepMicroOrg = {0, 5000}; // 5 millisecond
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  int iNumLoop       = 0;
  int iNumRepateRead = 5;
  int iRead          = 0;

  while (1)
  {
    iError = FLIGetTemperature( _hCam, &fTemperature);
    if (iError != 0)
    {
      printf("FliServer::setupCooling(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
      return 3;
    }

    if ( fTemperature <= fCoolingTemperature )
    {
      if ( ++iRead >= iNumRepateRead )
        break;
    }
    else
      iRead = 0;

    if ( (iNumLoop+1) % 200 == 0 )
      printf("Temperature *Updating*: %.1lf C\n", fTemperature );

    timespec timeValCur;
    clock_gettime( CLOCK_REALTIME, &timeValCur );
    int iWaitTime = (timeValCur.tv_nsec - timeVal1.tv_nsec) / 1000000 +
     ( timeValCur.tv_sec - timeVal1.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime > _iMaxCoolingTime ) break;

    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg;
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
    iNumLoop++;
  }

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fCoolingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  printf("Cooling Time = %6.1lf ms\n", fCoolingTime);

  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::setupCooling(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 4;
  }
  printf("Temperature After cooling: %.1lf C\n", fTemperature);

  if ( fTemperature > fCoolingTemperature )
  {
    printf("FliServer::setupCooling(): Cooling failed, final temperature = %.1lf C",
     fTemperature );
    return ERROR_COOLING_FAILURE;
  }

  return 0;
}

int FliServer::initCaptureTask()
{
  if ( _pTaskCapture != NULL )
    return 0;

  if ( ! _bDelayMode ) // Prompt mode doesn't need to initialize the capture task, because the task will be performed in the event handler
    return 0;

  _pTaskCapture = new Task(TaskObject("FliServer"));

  return 0;
}

int FliServer::runCaptureTask()
{
  if ( _CaptureState != CAPTURE_STATE_RUN_TASK )
  {
    printf( "FliServer::runCaptureTask(): _CaptureState = %d. Capture task is not initialized correctly\n",
     _CaptureState );
    return ERROR_INCORRECT_USAGE;
  }

  LockCameraData lockCaptureProcess(const_cast<char*>("FliServer::runCaptureTask(): Start data polling and processing" ));

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
  if ( _pDgOut == NULL )
  {
    printf( "FliServer::runCaptureTask(): Datagram or frame data have not been properly set up before the capture task\n" );
    resetFrameData(true);
    return ERROR_INCORRECT_USAGE;
  }

  //!!!debug
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */
  char      sTimeText[40];
  timespec  timeCurrent;
  time_t    timeSeconds ;

  int iFail = 0;
  do
  {
    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("FliServer::runCaptureTask(): Before waitForNewFrameAvailable(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    iFail  = waitForNewFrameAvailable();

    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("FliServer::runCaptureTask(): After waitForNewFrameAvailable(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    // Even if waitForNewFrameAvailable() failed, we still fill in the frame data with ShotId information
    iFail |= processFrame();

    //!!!debug
    clock_gettime( CLOCK_REALTIME, &timeCurrent );
    timeSeconds = timeCurrent.tv_sec;
    strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    printf("FliServer::runCaptureTask(): After processFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);
  }
  while (false);

  if ( iFail != 0 )
  {
    // set damage bit, and still keep the image data
    _pDgOut->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

    // // Old ways: delete image data and set the capture state to IDLE
    //resetFrameData(true);
    //return ERROR_FUNCTION_FAILURE;
  }

  /*
   * Set the damage bit if
   *   1. temperature status is not good
   *   2. sequence error happened in the current run
   */
  // Note: Dont send damage when temperature is high
  updateTemperatureData();
  //if ( updateTemperatureData() != 0 )
  //  _pDgOut->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

  _CaptureState = CAPTURE_STATE_DATA_READY;

  //!!!debug
  clock_gettime( CLOCK_REALTIME, &timeCurrent );
  timeSeconds = timeCurrent.tv_sec;
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
  printf("After capture: Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

  return 0;
}

int FliServer::startExposure()
{
  ++_iNumExposure; // update event counter

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
      printf( "FliServer::startExposure(): Previous image data has not been sent out\n" );
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

    printf( "FliServer::startExposure(): Capture task is running. It is impossible to start a new capture.\n" );

    /*
     * Here we don't reset the frame data, because the capture task is running and will use the data later
     */
    return ERROR_INCORRECT_USAGE; // No error for adaptive mode
  }

  //!!!debug
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */
  char      sTimeText[40];
  timespec  timeCurrent;
  time_t    timeSeconds ;

  ////!!!debug
  //clock_gettime( CLOCK_REALTIME, &timeCurrent );
  //timeSeconds = timeCurrent.tv_sec;
  //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
  //printf("FliServer::startExposure(): Before setupFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

  int iFail = 0;
  do
  {
    iFail = setupFrame();
    if ( iFail != 0 ) break;

    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("FliServer::startExposure(): After setupFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    iFail = startCapture();

    //!!!debug
    clock_gettime( CLOCK_REALTIME, &timeCurrent );
    timeSeconds = timeCurrent.tv_sec;
    strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    printf("FliServer::startExposure(): After startCapture(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);
  }
  while (false);

  if ( iFail != 0 )
  {
    resetFrameData(true);
    return ERROR_FUNCTION_FAILURE;
  }

  _CaptureState = CAPTURE_STATE_RUN_TASK;

  if (_bDelayMode)
    _pTaskCapture->call( &_routineCapture );
  else
    runCaptureTask();

  return 0;
}

int FliServer::getData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream

  if ( _CaptureState != CAPTURE_STATE_DATA_READY )
    return 0;

  /*
   * Check if the datagram and frame data have been properly set up
   *
   * Note: This condition should NOT happen normally. Here is a logical check
   * and will output warnings.
   */
  if ( _pDgOut == NULL )
  {
    printf( "FliServer::getData(): Datagram is not properly set up\n" );
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

  //dgOut.xtc = xtcOutBkp; // not okay for command
  dgOut.xtc.damage = xtcOutBkp.damage;
  dgOut.xtc.extent = xtcOutBkp.extent;

  unsigned char*  pFrameHeader  = (unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc);
  new (pFrameHeader) FliDataType(in->datagram().seq.stamp().fiducials(), _fReadoutTime);

  out       = _pDgOut;

  /*
   * Reset the frame data, without releasing the output data
   *
   * Note:
   *   1. _pDgOut will be set to NULL, so that the same data will never be sent out twice
   *   2. _pDgOut will not be released, because the data need to be sent out for use.
   */
  resetFrameData(false);

  // Delayed data sending for multiple fli cameras, to avoid creating a burst of traffic
  timeval timeSleepMicro = {0, 1000 * _iSleepInt}; // (_iSleepInt) milliseconds
  select( 0, NULL, NULL, NULL, &timeSleepMicro);

  return 0;
}

int FliServer::waitData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream

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
      printf( "FliServer::waitData(): Waiting time is too long. Skip the final data\n" );
      return ERROR_FUNCTION_FAILURE;
    }
  } // while (1)

  return getData(in, out);
}

int FliServer::waitForNewFrameAvailable()
{
  static timespec tsWaitStart;
  clock_gettime( CLOCK_REALTIME, &tsWaitStart );

  while (true)
  {
    int       iError;
    long int  iTimeRemain; // in ms
    iError = FLIGetExposureStatus(_hCam, &iTimeRemain);
    if (iError != 0)
    {
      printf("FliServer::waitForNewFrameAvailable(): FLIGetExposureStatus() failed. Error code %d: %s\n", iError, strerror(-iError));
      break;
    }

    if (iTimeRemain == 0)
      break;

    timeval timeSleepMicro = {0, iTimeRemain*1000};
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);

    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );

    int iWaitTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1000000 +
     ( tsCurrent.tv_sec - tsWaitStart.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime >= _iMaxReadoutTime )
    {
      printf( "FliServer::waitForNewFrameAvailable(): Readout time is longer than %d miliseconds. Capture is stopped\n",
       _iMaxReadoutTime );

      int iError;
      iError = FLICancelExposure(_hCam);
      if (iError != 0)
      {
        printf("FliServer::waitForNewFrameAvailable(): FLICancelExposure() failed. Error code %d: %s\n", iError, strerror(-iError));
      }

      // The  readout time (with incomplete data) will be reported in the framedata
      _fReadoutTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsCurrent.tv_sec - tsWaitStart.tv_sec ); // in seconds
      return 1;
    }
  } // while (true)

  uint8_t*  pLine       = (uint8_t*) _pDgOut + _iFrameHeaderSize;
  const int iLineWidth  = _iImageWidth * sizeof(uint16_t);
  for (int iY = 0; iY < _iImageHeight; ++iY, pLine += iLineWidth)
  {
    int iError = FLIGrabRow( _hCam, pLine, _iImageWidth);
    if (iError != 0)
    {
      printf("FliServer::waitForNewFrameAvailable(): FLIGrabRow() failed at line %d. Error code %d: %s\n", iY, iError, strerror(-iError));
      break;
    }
  }

  timespec tsWaitEnd;
  clock_gettime( CLOCK_REALTIME, &tsWaitEnd );

  _fReadoutTime = (tsWaitEnd.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsWaitEnd.tv_sec - tsWaitStart.tv_sec ); // in seconds

  // Report the readout time for the first few L1 events
  if ( _iNumExposure <= _iMaxEventReport )
    printf( "Readout time report [%d]: %.2f s  Non-exposure time %.2f s\n", _iNumExposure, _fReadoutTime,
      _fReadoutTime - _config.exposureTime());

  return 0;
}

int FliServer::processFrame()
{
  if ( _pDgOut == NULL )
  {
    printf( "FliServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  if ( _iNumExposure <= _iMaxEventReport ||  _iDebugLevel >= 5 )
  {
    unsigned char*  pFrameHeader   = (unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc);
    FliDataType* pFrame     = (FliDataType*) pFrameHeader;
    const uint16_t*     pPixel     = pFrame->data();
    //int                 iWidth   = (int) ( (_config.width()  + _config.binX() - 1 ) / _config.binX() );
    //int                 iHeight  = (int) ( (_config.height() + _config.binY() - 1 ) / _config.binY() );
    const uint16_t*     pEnd       = (const uint16_t*) ( (unsigned char*) pFrame->data() + _config.frameSize() );
    const uint64_t      uNumPixels = (uint64_t) (_config.frameSize() / sizeof(uint16_t) );

    uint64_t            uSum    = 0;
    uint64_t            uSumSq  = 0;
    for ( ; pPixel < pEnd; pPixel++ )
    {
      uSum   += *pPixel;
      uSumSq += ((uint32_t)*pPixel) * ((uint32_t)*pPixel);
    }

    printf( "Frame Avg Value = %.2lf  Std = %.2lf\n",
      (double) uSum / (double) uNumPixels,
      sqrt( (uNumPixels * uSumSq - uSum * uSum) / (double)(uNumPixels*uNumPixels)) );
  }

  return 0;
}

int FliServer::setupFrame()
{
  if ( _poolFrameData.numberOfFreeObjects() <= 0 )
  {
    printf( "FliServer::setupFrame(): Pool is full, and cannot provide buffer for new datagram\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  const int iFrameSize =_config.frameSize();

  /*
   * Set the output datagram pointer
   *
   * Note: This pointer will be used in processFrame(). In the case of delay mode,
   * processFrame() will be exectued in another thread.
   */
  InDatagram*& out = _pDgOut;
  //
  //  Fake a datagram header.  The real header will come with the L1Accept.
  //
  out =
    new ( &_poolFrameData ) CDatagram( TypeId(TypeId::Any,0), DetInfo(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0) );

  out->datagram().xtc.alloc( sizeof(Xtc) + iFrameSize );

  if ( _iDebugLevel >= 3 )
  {
    printf( "FliServer::setupFrame(): pool status: %d/%d allocated, datagram: %p\n",
     _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects(), _pDgOut  );
  }

  /*
   * Set frame object
   */
  unsigned char* pcXtcFrame = (unsigned char*) _pDgOut + sizeof(CDatagram);

  Xtc* pXtcFrame =
   new ((char*)pcXtcFrame) Xtc(_fliDataType, _src);
  pXtcFrame->alloc( iFrameSize );

  return 0;
}

int FliServer::resetFrameData(bool bDelOutDatagram)
{
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

int FliServer::setupROI()
{
  printf("ROI (%d,%d) W %d/%d H %d/%d ", _config.orgX(), _config.orgY(),
    _config.width(), _config.binX(), _config.height() , _config.binY());

  _iImageWidth  = _config.width()  / _config.binX();
  _iImageHeight = _config.height() / _config.binY();
  printf("image size: W %d H %d\n", _iImageWidth, _iImageHeight);

  int iError;

  iError = FLISetImageArea( _hCam, _config.orgX(), _config.orgY(),
    _config.orgX() + _iImageWidth, _config.orgY() + _iImageHeight);
  if (iError != 0)
  {
    printf("FliServer::setupROI(): FLISetImageArea() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  iError = FLISetHBin( _hCam, _config.binX());
  if (iError != 0)
  {
    printf("FliServer::setupROI(): FLISetHBin() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 2;
  }

  iError = FLISetVBin( _hCam, _config.binY());
  if (iError != 0)
  {
    printf("FliServer::setupROI(): FLISetVBin() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 3;
  }

  return 0;
}

int FliServer::updateTemperatureData()
{
  int     iError;
  double  fTemperature;
  iError = FLIGetTemperature( _hCam, &fTemperature);
  if (iError != 0)
  {
    printf("FliServer::updateTemperatureData(): FLIGetTemperature() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  /*
   * Set Info object
   */
  printf( "Detector Temperature report [%d]: %.1lf C\n", _iNumExposure, fTemperature );

  if ( _pDgOut == NULL )
  {
    printf( "FliServer::updateTemperatureData(): Datagram has not been allocated. No buffer to store the info data\n" );
  }
  else
  {
    FliDataType*  pFliData       = (FliDataType*) ((unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc));
    pFliData->setTemperature( (float) fTemperature );
  }

  if ( fTemperature >= _config.coolingTemp() + _fTemperatureHiTol ||
    fTemperature <= _config.coolingTemp() - _fTemperatureLoTol )
  {
    printf( "** FliServer::updateTemperatureData(): Detector temperature (%.1f C) is not fixed to the configuration (%.1f C)\n",
      fTemperature, _config.coolingTemp() );
    return 2;
  }

  return 0;
}

//int FliServer::checkSequence( const Datagram& datagram )
//{
//  /*
//   * Check for sequence error
//   */
//  const ClockTime clockCurDatagram  = datagram.seq.clock();
//
//  /*
//   * Note: ClockTime.seconds() and ClockTime.nanoseconds() are unsigned values, and
//   *   the difference of two unsigned values will be also interpreted as an unsigned value.
//   *   We need to convert the computed differences to signed values by using (int) operator.
//   *   Otherwise, negative values will be interpreted as large positive values.
//   */
//  float     fDeltaTime        = (int) ( clockCurDatagram.seconds() - _clockPrevDatagram.seconds() ) +
//   (int) ( clockCurDatagram.nanoseconds() - _clockPrevDatagram.nanoseconds() ) * 1.0e-9f;
//
//  if ( fDeltaTime < _fPrevReadoutTime * _fEventDeltaTimeFactor )
//  {
//    // Report the error for the first few L1 events
//    if ( _iNumExposure <= _iMaxEventReport )
//      printf( "** FliServer::checkSequence(): Sequence error. Event delta time (%.2fs) < Prev Readout Time (%.2fs) * Factor (%.2f)\n",
//        fDeltaTime, _fPrevReadoutTime, _fEventDeltaTimeFactor );
//
//    _bSequenceError = true;
//    return ERROR_SEQUENCE_ERROR;
//  }
//
//  if ( _iDebugLevel >= 3 )
//  {
//    if ( _clockPrevDatagram.seconds() == 0 && _clockPrevDatagram.nanoseconds() == 0 )
//      fDeltaTime = 0;
//
//    printf( "FliServer::checkSequence(): Event delta time (%.2fs), Prev Readout Time (%.2fs), Factor (%.2f)\n",
//      fDeltaTime, _fPrevReadoutTime, _fEventDeltaTimeFactor );
//  }
//  _clockPrevDatagram = clockCurDatagram;
//
//  return 0;
//}

int FliServer::getCamera(flidomain_t domain, int iCameraId, std::string& strCameraDev)
{
  char **lsCamera;

  int iError = FLIList( domain | FLIDEVICE_CAMERA, &lsCamera);
  if (iError != 0)
  {
    printf("FliServer::getCamera(): FLIList() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  if (lsCamera == NULL)
  {
    printf("FliServer::getCamera(): Not camera found\n");
    return 2;
  }

  int iNumCamera = 0;
  while (lsCamera[iNumCamera] != NULL)
  {
    if (iNumCamera == iCameraId)
    {
      strCameraDev = lsCamera[iNumCamera];
      size_t iFindPos = strCameraDev.find(';');
      if (iFindPos != string::npos)
        strCameraDev.erase(iFindPos);
      printf("** Use camera [%d] %s\n", iNumCamera, strCameraDev.c_str());
    }
    else
      printf("Found camera [%d] %s\n", iNumCamera, lsCamera[iNumCamera]);
    ++iNumCamera;
  }

  iError = FLIFreeList( lsCamera );
  if (iError != 0)
  {
    printf("FliServer::getCamera(): FLIFreeList() failed. Error code %d: %s\n", iError, strerror(-iError));
    return 1;
  }

  if (iCameraId < 0 || iCameraId >= iNumCamera)
  {
    printf("FliServer::getCamera(): Cannot find camera %d\n", iCameraId);
    return 3;
  }

  return 0;
}

/*
 * Definition of private static consts
 */
const int       FliServer::_iMaxCoolingTime;
const int       FliServer::_fTemperatureHiTol;
const int       FliServer::_fTemperatureLoTol;
const int       FliServer::_iFrameHeaderSize      = sizeof(CDatagram) + sizeof(Xtc) + sizeof(FliDataType);
const int       FliServer::_iMaxFrameDataSize     = _iFrameHeaderSize + 4096*4096*2;
const int       FliServer::_iPoolDataCount;
const int       FliServer::_iMaxReadoutTime;
const int       FliServer::_iMaxThreadEndTime;
const int       FliServer::_iMaxLastEventTime;
const int       FliServer::_iMaxEventReport;
const float     FliServer::_fEventDeltaTimeFactor = 1.01f;

/*
 * Definition of private static data
 */
pthread_mutex_t FliServer::_mutexPlFuncs = PTHREAD_MUTEX_INITIALIZER;


FliServer::CaptureRoutine::CaptureRoutine(FliServer& server) : _server(server)
{
}

void FliServer::CaptureRoutine::routine(void)
{
  _server.runCaptureTask();
}

} //namespace Pds
