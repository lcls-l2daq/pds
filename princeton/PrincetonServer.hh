#ifndef PRINCETON_SERVER_HH
#define PRINCETON_SERVER_HH

#include <time.h>
#include <pthread.h>
#include <string>
#include <stdexcept>

#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pdsdata/princeton/FrameV1.hh"
#include "pds/xtc/InDatagram.hh"

namespace Pds 
{
  
class PrincetonServer
{
public:
  PrincetonServer(bool bUseCaptureThread, bool bStreamMode, std::string sFnOutput, int iDebugLevel);
  ~PrincetonServer();
  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   captureStart(int iShotId);
  int   captureEnd(InDatagram* in, InDatagram* out);
  
private:

  int   initCamera();
  int   deinitCamera();
  int   startContinuousCapture();
  void  abortAndResetCamera();
  
  int   setupCooling();  
  int   validateCameraSettings(Princeton::ConfigV1& config);
  void  setupROI(rgn_type& region);
  int   isCaptureInProgress();
  int   isNewFrameAvailable();
  
  int   initControlThread();
  int   runMonitorThread();
  int   runCaptureThread();
  
  /*
   * Initial settings
   */
  bool        _bUseCaptureThread;
  bool        _bStreamMode;
  std::string _sFnOutput;
  int         _iDebugLevel;
  
  /*
   * Internal data
   */
  short               _hCam;
  int                 _iCurrentShotId;
  Princeton::ConfigV1 _configCamera;
  unsigned char*      _pCircBufferWithHeader; // special value: NULL -> Camera has not been configured  
  int                 _iCameraAbortAndReset;  // 0 -> normal, 1 -> aborting
  int                 _iEventCaptureEnd;      // 0 -> normal, 1 -> capture end event triggered
  
  /*
   * private static consts
   */  
  static const short  _iCoolingTemperature    = -1000; // = -10 degrees Celsius
  static const int    _iMaxCoolingTime        = 10000; // in miliseconds
  static const int    _iCircBufFrameCount     = 5;
  static const int16  _modeExposure           = BULB_MODE;
  static const int    _iMaxExposureTime       = 30000; // Limit exposure time to prevent CCD from burning
  
  static void* threadEntryMonitor(void * pServer);
  static void* threadEntryCapture(void * pServer);
  
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
