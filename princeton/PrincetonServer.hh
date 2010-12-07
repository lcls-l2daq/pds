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
  PrincetonServer(int iCamera, bool bInitTest, const Src& src, int iDebugLevel);
  ~PrincetonServer();
  
  int   mapCamera();  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   beginRunCamera();
  int   endRunCamera();  
  int   enableCamera();
  int   disableCamera();  
  int   onEventReadout();
  int   getDelayData(InDatagram* in, InDatagram*& out);
  int   getLastDelayData(InDatagram* in, InDatagram*& out);
  int   checkReadoutEventCode(unsigned code);
  
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
  static const int      _iMaxCoolingTime        = 58000;        // in miliseconds
  static const int      _iTemperatureHiTol      = 100;          // 1 degree Celcius
  static const int      _iTemperatureLoTol      = 20000;        // 200 degree Celcius -> Do not use Low Tolerance now
  static const int      _iClockSavingExpTime    = 24*60*60*1000;// 24 hours -> Long exposure time for clock saving
  static const int      _iFrameHeaderSize;                      // Buffer header used to store the CDatagram, Xtc and FrameV1 object
  static const int      _iInfoSize;                             // For storing temperature infomation
  static const int      _iMaxFrameDataSize;                     // Buffer for 4 Mega (image pixels) x 2 (bytes per pixel) + 
                                                                //   info size + header size
  static const int      _iPoolDataCount         = 5;            // 4 buffer for traffic shaping, 1 buffer for capture thread (in delay mode)
  static const int      _iMaxReadoutTime        = 120000;        // Max readout time
  static const int      _iMaxThreadEndTime      = 120000;        // Max thread terminating time (in ms)
  static const int      _iMaxLastEventTime      = 120000;        // Max readout time for the last (diable) event
  static const int      _iMaxEventReport        = 20;           // Only report some statistics and non-critical errors in the first few L1 events
  static const float    _fEventDeltaTimeFactor;                 // Event delta time factor, for detecting sequence error  

  /*
   * private functions
   */
  int   initCamera();
  int   deinitCamera();  
  
  int   initCapture();
  int   startCapture();
  int   deinitCapture();  

  int   runCaptureTask();
  
  int   initCameraSettings(Princeton::ConfigV1& config);

  int   initTest();
  
  int   initClockSaving();
  int   deinitClockSaving();
  /*
   * Frame handling functions
   */
  int   setupFrame();
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
  const int           _iCamera;
  const bool          _bInitTest;
  const Src           _src;
  const int           _iDebugLevel;
  
  /*
   * Camera basic status control
   */
  short               _hCam;  
  bool                _bCameraInited;
  bool                _bCaptureInited;
  bool                _bClockSaving;
  
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
