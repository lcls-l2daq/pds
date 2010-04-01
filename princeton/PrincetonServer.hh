#ifndef PRINCETON_SERVER_HH
#define PRINCETON_SERVER_HH

#include <time.h>
#include <pthread.h>
#include <string>
#include <stdexcept>

#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pdsdata/princeton/FrameV1.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"

namespace Pds 
{

class Task;
class PrincetonServer;

class PrincetonServer
{
public:
  PrincetonServer(bool bUseCaptureTask, const Src& src, int iDebugLevel);
  ~PrincetonServer();
  
  int   mapCamera();  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   beginRunCamera();
  int   endRunCamera();  
  int   onEventReadoutPrompt(int iShotId, InDatagram* in, InDatagram*& out);
  int   onEventReadoutDelay(int iShotId, InDatagram* in);  
  int   getDelayData(InDatagram* in, InDatagram*& out);
  int   getLastDelayData(InDatagram* in, InDatagram*& out);
  int   checkReadoutEventCode(InDatagram* in);
  
  enum  ErrorCodeEnum
  {
    ERROR_INVALID_ARGUMENTS = 1,
    ERROR_FUNCTION_FAILURE  = 2,
    ERROR_INCORRECT_USAGE   = 3,
    ERROR_LOGICAL_FAILURE   = 4,
    ERROR_PVCAM_FUNC_FAIL   = 5,
    ERROR_SERVER_INIT_FAIL  = 6,
    ERROR_INVALID_CONFIG    = 7,
    ERROR_COOLING_FAILURE   = 8,
    ERROR_TEMPERATURE_HIGH  = 9,
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
  static const int      _iMaxCoolingTime        = 1000;         // in miliseconds
  static const int      _iTemperatureTolerance  = 100;          // 1 degree Fahrenheit
  static const int      _iFrameHeaderSize;                      // Buffer header used to store the CDatagram, Xtc and FrameV1 object
  static const int      _iMaxFrameDataSize;                     // Buffer for 4 Mega (image pixels) x 2 (bytes per pixel) + header size
  static const int      _iPoolDataCount         = 2;
  static const int      _iMaxReadoutTime        = 5000;         // Max readout time
  static const int      _iMaxThreadEndTime      = 3000;         // Max thread terminating time (in ms)
  static const int      _iMaxLastEventTime      = 3000;         // Max thread terminating time (in ms)
  static const int      _iMaxEventReport        = 20;           // Only report some statistics and non-critical errors in the first few L1 events
  static const float    _fEventDeltaTimeFactor;                 // Event delta time factor, for detecting sequence error  

  /*
   * private classes
   */
  class CaptureRoutine : public Routine 
  {
  public:
    CaptureRoutine(PrincetonServer& server);
    void routine(void);
  private:
    PrincetonServer& _server;
  };  
  
  /*
   * private functions
   */
  int   initCamera();
  int   deinitCamera();  
  
  int   initCapture();
  int   startCapture();
  int   deinitCapture();  

  int   initCaptureTask();
  int   runCaptureTask();
  
  int   initCameraSettings(Princeton::ConfigV1& config);

  /*
   * Frame handling functions
   */
  int   setupFrame(InDatagram* in, InDatagram*& out);  
  int   waitForNewFrameAvailable();
  int   processFrame();
  int   resetFrameData(bool bDelOutDatagram);

  int   setupCooling();    
  int   checkTemperature();  
  int   checkSequence( const Datagram& datagram );
  void  setupROI(rgn_type& region);
  
  /*
   * Initial settings
   */
  const bool          _bDelayMode;
  const Src           _src;
  const int           _iDebugLevel;
  
  /*
   * Camera basic status control
   */
  short               _hCam;  
  bool                _bCameraInited;
  bool                _bCaptureInited;
  
  /*
   * Event sequence/traffic control
   */
  float               _fPrevReadoutTime;// in seconds. Used to filter out events that are coming too fast
  bool                _bSequenceError;  
  ClockTime           _clockPrevDatagram;
  int                 _iNumL1Event;
  
  /*
   * Config data
   */ 
  Princeton::ConfigV1 _configCamera; 
  
  /*
   * Per-frame data
   */
  int                 _iCurShotId;
  float               _fReadoutTime;    // in seconds
      
  /*
   * Buffer control
   */
  GenericPool         _poolFrameData;
  InDatagram*         _pDgOut;          // Datagram for outtputing to the Princeton Manager
    
  /*
   * Capture Task Control
   */
  CaptureStateEnum    _CaptureState;    // 0 -> idle, 1 -> start data polling/processing, 2 -> data ready  
  Task*               _pTaskCapture;
  CaptureRoutine      _routineCapture;
      
  /*
   * Thread syncronization (lock/unlock) functions
   */
  inline static void lockCameraData(char* sDescription)
  {
    if ( pthread_mutex_lock(&_mutexPlFuncs) )
      printf( "PrincetonServer::lockCameraData(): pthread_mutex_timedlock() failed for %s\n", sDescription );
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

class PrincetonServerException : public std::runtime_error
{
public:
  explicit PrincetonServerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} //namespace Pds 

#endif
