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

namespace Pds 
{
  
class PrincetonServer
{
public:
  PrincetonServer(bool bUseCaptureThread, bool bStreamMode, std::string sFnOutput, const Src& src, int iDebugLevel);
  ~PrincetonServer();
  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   captureStart(int iShotIdStart);
  int   captureEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   getMakeUpData(InDatagram* in, InDatagram*& out);
  
private:

  int   initCamera();
  int   deinitCamera();
  int   startContinuousCapture();
  void  abortAndResetCamera();

  int   initControlThreads();
  int   runMonitorThread();
  int   runCaptureThread();

  int   checkInitSettings();    
  int   validateCameraSettings(Princeton::ConfigV1& config);
  int   setupCooling();    
  int   isExposureInProgress();
  int   waitForNewFrameAvailable();
  int   processFrame(InDatagram* in, InDatagram*& out);
  int   writeFrameToFile(const Datagram& dgOut);
  
  void  discardExcessFrames(uns32 uNumBufferAvailable);
  void  setupROI(rgn_type& region);
  void  checkTemperature();  
  
  /*
   * Initial settings
   */
  const bool        _bUseCaptureThread;
  const bool        _bStreamMode;
  const std::string _sFnOutput;
  const Src         _src;
  const int         _iDebugLevel;
  
  /*
   * Internal data
   */
  short               _hCam;
  int                 _iCurShotIdStart;
  int                 _iCurShotIdEnd;
  float               _fReadoutTime;          // in seconds
  Princeton::ConfigV1 _configCamera; 
  unsigned char*      _pCircBufferWithHeader; // special value: NULL -> Camera has not been configured  
  int                 _iCameraAbortAndReset;  // 0 -> normal, 1 -> aborting
  int                 _iEventCaptureEnd;      // 0 -> normal, 1 -> capture end event triggered
  bool                _bForceCameraReset;     
  int                 _iTemperatureStatus;    // 0 -> normal, 1 -> too high
  Datagram            _dgEvent;
  InDatagram*         _pDgOut;
  
  /*
   * private static consts
   */  
  static const int      _iMaxCoolingTime        = 10000;  // in miliseconds
  static const int      _iTemperatureTolerance  = 100;    // 1 degree Fahrenheit
  static const int      _iCircBufFrameCount     = 5;
  static const int      _iMaxExposureTime       = 30000;  // Limit exposure time to prevent CCD from burning
  static const int      _iMaxReadoutTime        = 3000;   // Max readout time          
  static const timespec _tmLockTimeout;                   // timeout for acquiring the lock
    
  /*
   * private static functions
   */
  static void* threadEntryMonitor(void * pServer);
  static void* threadEntryCapture(void * pServer);

  inline static void lockPlFunc()
  {
    if ( !pthread_mutex_timedlock(&_mutexPlFuncs, &_tmLockTimeout) )
      printf( "PrincetonServer::lockPlFunc(): pthread_mutex_timedlock() failed\n" );
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
