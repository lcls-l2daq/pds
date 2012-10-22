#ifndef FLI_SERVER_HH
#define FLI_SERVER_HH

#include <time.h>
#include <pthread.h>
#include <string>
#include <stdexcept>

#include "fli/include/libfli.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/FliConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"

namespace Pds 
{

class Task;
class FliServer;

class FliServer
{
public:
  FliServer(int iCamera, bool bUseCaptureTask, bool bInitTest, const Src& src, std::string sConfigDb, int iSleepInt, int iDebugLevel);
  ~FliServer();
  
  int   map();  
  int   config(FliConfigType& config, std::string& sConfigWarning);
  int   unconfig();
  int   beginRun();
  int   endRun();  
  int   beginCalibCycle();
  int   endCalibCycle();  
  int   enable();
  int   disable();  
  int   startExposure();
  int   getData (InDatagram* in, InDatagram*& out);
  int   waitData(InDatagram* in, InDatagram*& out);
  FliConfigType&
        config() { return _config; }
  
  enum  ErrorCodeEnum
  {
    ERROR_INVALID_ARGUMENTS = 1,
    ERROR_FUNCTION_FAILURE  = 2,
    ERROR_INCORRECT_USAGE   = 3,
    ERROR_LOGICAL_FAILURE   = 4,
    ERROR_SDK_FUNC_FAIL     = 5,
    ERROR_SERVER_INIT_FAIL  = 6,
    ERROR_INVALID_CONFIG    = 7,
    ERROR_COOLING_FAILURE   = 8,
    ERROR_TEMPERATURE       = 9,
    ERROR_SEQUENCE_ERROR    = 10,
  };
  
private:
  /*  
   * private enum
   */  
  enum CaptureStateEnum
  {
    CAPTURE_STATE_IDLE        = 0,
    CAPTURE_STATE_RUN_TASK    = 1,
    CAPTURE_STATE_DATA_READY  = 2,
  };

  /*  
   * private static consts
   */    
  static const int      _iMaxCoolingTime        = 100;        // in miliseconds
  static const int      _fTemperatureHiTol      = 1;          // 1 degree Celcius
  static const int      _fTemperatureLoTol      = 200;        // 200 degree Celcius -> Do not use Low Tolerance now
  static const int      _iClockSavingExpTime    = 24*60*60*1000;// 24 hours -> Long exposure time for clock saving
  static const int      _iFrameHeaderSize;                      // Buffer header used to store the CDatagram, Xtc and FrameV1 object
  static const int      _iMaxFrameDataSize;                     // Buffer for 4 Mega (image pixels) x 2 (bytes per pixel) + 
                                                                //   info size + header size
  static const int      _iPoolDataCount         = 5;            // 4 buffer for traffic shaping, 1 buffer for capture thread (in delay mode)
  static const int      _iMaxReadoutTime        = 120000;        // Max readout time
  static const int      _iMaxThreadEndTime      = 120000;        // Max thread terminating time (in ms)
  static const int      _iMaxLastEventTime      = 120000;        // Max readout time for the last event
  static const int      _iMaxEventReport        = 20;           // Only report some statistics and non-critical errors in the first few L1 events
  static const float    _fEventDeltaTimeFactor;                 // Event delta time factor, for detecting sequence error  

  /*
   * private classes
   */
  class CaptureRoutine : public Routine 
  {
  public:
    CaptureRoutine(FliServer& server);
    void routine(void);
  private:
    FliServer& _server;
  };  
  
  /*
   * private functions
   */
   
  /*
   * camera control functions
   */ 
  int   init();
  int   deinit();  
  
  int   initCapture();
  int   startCapture();
  int   deinitCapture();  

  int   initCaptureTask(); // for delay mode use only
  int   runCaptureTask();
  
  int   initCameraBeforeConfig();
  int   configCamera(FliConfigType& config, std::string& sConfigWarning);

  int   initTest();
  int   setupROI();
  
  static int  getCamera(flidomain_t domain, int iCameraId, std::string& strCameraDev);
  
  /*
   * Frame handling functions
   */
  int   setupFrame();
  //int   setupFrame(InDatagram* in, InDatagram*& out); //!!delay mode
  int   waitForNewFrameAvailable();
  int   processFrame();
  int   resetFrameData(bool bDelOutDatagram);

  int   setupCooling(double fCoolingTemperature);    
  int   updateTemperatureData();  
  //int   checkSequence( const Datagram& datagram );
  
  /*
   * Initial settings
   */
  const int           _iCamera;
  const bool          _bDelayMode;
  const bool          _bInitTest;
  const Src           _src;
  const std::string   _sConfigDb;
  const int           _iSleepInt;
  const int           _iDebugLevel;
  
  /*
   * Camera basic status control
   */
  flidev_t            _hCam;  
  bool                _bCameraInited;
  bool                _bCaptureInited;
  
  /*
   * Camera hardware settings
   */
  int                 _iDetectorWidth;
  int                 _iDetectorHeight; 
  int                 _iDetectorOrgX;
  int                 _iDetectorOrgY;    
  int                 _iDetectorDataWidth;
  int                 _iDetectorDataHeight;      
  int                 _iImageWidth;
  int                 _iImageHeight;
  
  
  /*
   * Event sequence/traffic control
   */
  float               _fPrevReadoutTime;// in seconds. Used to filter out events that are coming too fast
  bool                _bSequenceError;  
  ClockTime           _clockPrevDatagram;
  int                 _iNumExposure;
  
  /*
   * Config data
   */ 
  FliConfigType _config; 
  
  /*
   * Per-frame data
   */
  float               _fReadoutTime;    // in seconds
      
  /*
   * Buffer control
   */
  GenericPool         _poolFrameData;
  InDatagram*         _pDgOut;          // Datagram for outtputing to the Fli Manager
    
  /*
   * Capture Task Control
   */
  CaptureStateEnum    _CaptureState;    // 0 -> idle, 1 -> start data polling/processing, 2 -> data ready  
  Task*               _pTaskCapture;    // for delay mode use
  CaptureRoutine      _routineCapture;  // for delay mode use
      
  /*
   * Thread syncronization (lock/unlock) functions
   */
  inline static void lockCameraData(char* sDescription)
  {
    if ( pthread_mutex_lock(&_mutexPlFuncs) )
      printf( "FliServer::lockCameraData(): pthread_mutex_timedlock() failed for %s\n", sDescription );
  }
  inline static void releaseLockCameraData()
  {
    pthread_mutex_unlock(&_mutexPlFuncs);
  }
  
  class LockCameraData
  {
  public:
    LockCameraData(char* sDescription) 
    {
      lockCameraData(sDescription);
    }
    
    ~LockCameraData() 
    { 
      releaseLockCameraData(); 
    }
  };
    
  /*
   * private static data
   */
  static pthread_mutex_t _mutexPlFuncs;
};

class FliServerException : public std::runtime_error
{
public:
  explicit FliServerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} //namespace Pds 

#endif
