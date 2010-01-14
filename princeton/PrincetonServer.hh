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
  
private:

  int   initCamera();
  int   deinitCamera();
  int   startContinuousCapture();
  void  abortAndResetCamera();
  
  int   setupCooling();  
  int   validateCameraSettings(Princeton::ConfigV1& config);
  void  setupROI(rgn_type& region);
  int   isExposureInProgress();
  int   isNewFrameAvailable();
  int   waitForNewFrameAvailable(uns32& uNumBufferAvailable);
  int   processFrame(uns32 uNumBufferAvailable, InDatagram* in, InDatagram*& out);
  int   checkInitSettings();
  
  int   initControlThread();
  int   runMonitorThread();
  int   runCaptureThread();
  
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
  DetInfo             _detInfo;
  
  /*
   * private static consts
   */  
  static const int    _iMaxCoolingTime        = 10000;  // in miliseconds
  static const int    _iCircBufFrameCount     = 5;
  static const int    _iMaxExposureTime       = 30000;  // Limit exposure time to prevent CCD from burning
  static const int    _iMaxReadoutTime        = 3000;   // Max readout time          
  
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
