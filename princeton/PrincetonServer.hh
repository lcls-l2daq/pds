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
  int   onEventReadoutPrompt(int iShotId, InDatagram* in, InDatagram*& out);
  int   onEventReadoutDelay(int iShotId);  
  int   getDelayData(InDatagram* in, InDatagram*& out);
  int   getLastDelayData(InDatagram* in, InDatagram*& out);

private:
  /*  
   * private static consts
   */  
  static const int      _iMaxCoolingTime        = 1000;        // in miliseconds
  static const int      _iTemperatureTolerance  = 100;          // 1 degree Fahrenheit
  static const int      _iFrameHeaderSize;                      // Buffer header used to store the CDatagram, Xtc and FrameV1 object
  static const int      _iMaxFrameDataSize;                     // Buffer for 4 Mega (image pixels) x 2 (bytes per pixel) + header size
  static const int      _iPoolDataCount         = 2;
  static const int      _iMaxExposureTime       = 10000;        // Limit exposure time to prevent CCD from burning
  static const int      _iMaxReadoutTime        = 3000;         // Max readout time // !! debug - set to 3s for testing
  static const int      _iMaxThreadEndTime      = 2000;      // Max thread terminating time (in ms)
  static const int      _iMaxLastEventTime      = 1000;         // Max thread terminating time (in ms)
  
  /*
   * private functions
   */
  int   initCamera();
  int   deinitCamera();  
  int   initCapture();
  int   startCapture();
  int   deinitCapture();  

  int   initCaptureThread();
  int   runCaptureThread();

  int   checkInitSettings();    
  int   initCameraSettings(Princeton::ConfigV1& config);
  int   setupCooling();    
  int   waitForNewFrameAvailable();
  int   processFrame(InDatagram* in, InDatagram*& out);
  int   writeFrameToFile(const Datagram& dgOut);  
  int   resetFrameData();
  int   checkTemperature();  
  void  setupROI(rgn_type& region);
  
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
  int                 _iThreadStatus;   // 0: No thread generated yet, 1: One thread has been created, 2: Two thread has been created
  int                 _iThreadCommand;  // 0: Nothing, 1: End Thread
  
  /*
   * Config data
   */ 
  Princeton::ConfigV1 _configCamera; 
  
  /*
   * Per-frame data
   */
  int                 _iCurShotId;
  float               _fReadoutTime;          // in seconds
      
  /*
   * Buffer control
   */
  GenericPool         _poolEmptyData;          // For storing a temporary datagram (for capture thread)
  GenericPool         _poolFrameData;
  unsigned char*      _pCurDatagram;
    
  /*
   * Capture Thread Control and I/O variables
   */
  int                 _iEventCaptureEnd;      // 0 -> normal, 1 -> capture end event triggered
  InDatagram*         _pDgOut;                // Datagram for outtputing to the Princeton Manager
      
  /*
   * private static functions
   */
  static void* threadEntryCapture(void * pServer);

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
