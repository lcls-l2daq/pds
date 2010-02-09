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

namespace Pds 
{
  
class PrincetonServer
{
public:
  PrincetonServer(bool bUseCaptureThread, bool bStreamMode, std::string sFnOutput, const Src& src, int iDebugLevel);
  ~PrincetonServer();
  
  int   mapCamera();  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   beginRunCamera();
  int   endRunCamera();  
  int   onEventShotIdStart(int iShotIdStart);
  int   onEventShotIdEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   onEventShotIdUpdate(int iShotIdStart, int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   getMakeUpData(InDatagram* in, InDatagram*& out);

private:
  /*
   * private static consts
   */  
  static const int      _iMaxCoolingTime        = 10000;        // in miliseconds
  static const int      _iTemperatureTolerance  = 100;          // 1 degree Fahrenheit
  static const int      _iFrameHeaderSize;                      // Buffer header used to store the CDatagram, Xtc and FrameV1 object
  static const int      _iMaxFrameDataSize;                     // Buffer for 4 Mega (image pixels) x 2 (bytes per pixel) + header size
  static const int      _iCircPoolCount         = 2;
  static const int      _iMaxExposureTime       = 10000;        // Limit exposure time to prevent CCD from burning
  static const int      _iMaxReadoutTime        = 3000;         // Max readout time // !! debug - set to 3s for testing

  /*
   * private functions
   */
  int   initCamera();
  int   deinitCamera();  
  void  abortAndResetCamera();
  int   initCapture();
  int   startCapture(int iBufferIndex);
  int   deinitCapture();  

  int   initControlThreads();
  int   runMonitorThread();
  int   runCaptureThread();

  int   checkInitSettings();    
  int   initCameraSettings(Princeton::ConfigV1& config);
  int   setupCooling();    
  int   isExposureInProgress();
  int   waitForNewFrameAvailable();
  int   processFrame(InDatagram* in, InDatagram*& out);
  int   writeFrameToFile(const Datagram& dgOut);  
  void  setupROI(rgn_type& region);
  void  checkTemperature();  
  void  updateCameraIdleTime();
  
  /*
   * Initial settings
   */
  const bool          _bUseCaptureThread;
  const bool          _bStreamMode;
  const std::string   _sFnOutput;
  const Src           _src;
  const int           _iDebugLevel;
  
  /*
   * Camera basic status control
   */
  short               _hCam;  
  bool                _bCameraInited;
  bool                _bCaptureInited;
  int                 _iThreadStatus;   // 0: No thread generated yet, 1: Thread has been created, 2: Thread is notified to terminte

  /*
   * Camera Reset and Monitor Thread control variables
   */
  timespec            _tsPrevIdle;            // timestamp for the previous camera idle time
  int                 _iCameraAbortAndReset;  // 0 -> normal, 1 -> resetting in progress, 2 -> reset complete
  bool                _bForceCameraReset;     
  int                 _iTemperatureStatus;    // 0 -> normal, 1 -> too high  
  bool                _bImageDataReady;
  
  /*
   * Config data
   */ 
  Princeton::ConfigV1 _configCamera; 
  
  /*
   * Per-frame data
   */
  int                 _iCurShotIdStart;
  int                 _iCurShotIdEnd;
  float               _fReadoutTime;          // in seconds
      
  /*
   * Buffer control
   */
  GenericPool*        _lpCircPool       [_iCircPoolCount];
  unsigned char*      _lpDatagramBuffer [_iCircPoolCount];
  uns32               _uFrameSize;
  int                 _iCurPoolIndex;         // Used to locate the current pool inside the circular pool buffer
  int                 _iNextPoolIndex;        // The next pool index 
  GenericPool         _poolDatagram;          // For storing a temporary datagram (for capture thread)
    
  /*
   * Capture Thread Control and I/O variables
   */
  int                 _iEventCaptureEnd;      // 0 -> normal, 1 -> capture end event triggered
  Datagram            _dgEvent;               // Event header from Princeton Manager
  InDatagram*         _pDgOut;                // Datagram for outtputing to the Princeton Manager
      
  /*
   * private static functions
   */
  static void* threadEntryMonitor(void * pServer);
  static void* threadEntryCapture(void * pServer);

  /*
   * Thread syncronization (lock/unlock) functions
   */
  inline static void lockPlFunc(char* sDescription)
  {
    if ( pthread_mutex_lock(&_mutexPlFuncs) )
      printf( "PrincetonServer::lockPlFunc(): pthread_mutex_timedlock() failed for %s\n", sDescription );
  }
  inline static void releaseLockPlFunc()
  {
    pthread_mutex_unlock(&_mutexPlFuncs);
  }
  
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
