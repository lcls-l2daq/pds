#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/OceanOpticsDataType.hh"

#include "liboopt/liboopt.hh"
#include "OceanOpticsServer.hh"
#include "histreport.hh"

/*
 * local functions
 */
static int initSpectraCheck();
static int checkSpectraInfo(OceanOpticsDataType* pData);
static int finalSpectraReport();
static int printSpectraHeaderInfo(LIBOOPT::SpectraPostheader* pSpectraPostheader);

/*
 * local variables - for data statistics collection
 */

static const double fLongIntervalRatio  = 1.5;
static const int    iReportInterval     = 60; // in seconds
static int          iPostheaderOffset;
static int          iNumLongInterval    = 0;
static HistReport   histLongIntervalShot(0,30,1);
static HistReport   histFetchInterval(0,32,0.5);
static HistReport   histTimeFrame(0,32,0.1);
static HistReport   histTimeStart(0,32,0.5);
static HistReport   histTimeInterval(0,32,0.5);
static HistReport   histTimeFirstData(0,32,0.5);
static HistReport   histTimeGetData(0,0.5,0.1);
static HistReport   histTimeGap(0,0.05,0.005);
static HistReport   histNumSpectraInQueue(0,64,1);

static struct timespec      tsPrevFetch;
static struct timespec      tsPrevReport;
static LIBOOPT::timespec64  tsPrevInit;
static LIBOOPT::timespec64  tsPrevEnd;

namespace Pds
{

OceanOpticsServer::OceanOpticsServer(const Src& client, int iDevice, int iDebugLevel)
   : _xtc( _oceanOpticsDataType, client ), _count(0), _iDebugLevel(iDebugLevel)
{
  _xtc.extent = sizeof(Xtc) + sizeof(OceanOpticsDataType);  
  
  char sDevOopt[32];
  sprintf(sDevOopt, "/dev/oopt%d", iDevice);
  int fdDevice = open(sDevOopt, O_RDWR);  
  
  if (_iDebugLevel >= 1)
    printf("Opening %s. fd = %d\n", sDevOopt, fdDevice);
  
  char sError[128];
  if (fdDevice == -1)
  {    
    sprintf(sError, "OceanOpticsServer::OceanOpticsServer(): open() failed. [%d] %s", errno, strerror(errno));
    throw OceanOpticsServerException(sError);
  }

  if (LIBOOPT::initHR4000(fdDevice) != 0)
  {
    close(fdDevice);
    throw OceanOpticsServerException("OceanOpticsServer::OceanOpticsServer(): initHR4000() failed");
  }
  
  LIBOOPT::SpecStatus status;
  if (LIBOOPT::queryStatus(fdDevice, status) != 0)
  {
    close(fdDevice);
    throw OceanOpticsServerException("OceanOpticsServer::OceanOpticsServer(): queryStatus() failed");
  }
  printf("Device Status:\n");
  LIBOOPT::printStatus(status);     
        
  fd(fdDevice);  
}

OceanOpticsServer::~OceanOpticsServer()
{
  int iError = LIBOOPT::clearDataBuffer(fd());
  if (iError != 0)
    printf("OceanOpticsServer::~OceanOpticsServer(): clearDataBuffer() failed: %d\n", iError);
  
  if (fd() >= 0)
  {
    iError = close(fd());
    if (iError == -1)
      printf("OceanOpticsServer::~OceanOpticsServer(): close() failed. %s\n", strerror(errno));
    fd(-1);
  }
  
  printf("device closed successfully.\n");
}

bool OceanOpticsServer::devicePresent() const
{
  return (fd() >= 0);
}

unsigned OceanOpticsServer::configure(OceanOpticsConfigType& config)
{
  if (!devicePresent())
    return ERR_DEVICE_NOT_PRESENT;
    
  // todo: read out the other config values (such as calibration coefficients) and pass back 
  // note: read the values before setting the exposure is better for the device
  
  /*
   * Read back the config values. Fill them into the config object.   
   */  
  printf("Reading config variables:\n");
  const int iWaveLenCalibStart  = 1;
  const int iWaveLenCalibLen    = 4;
  const int iftrayLightConstant = 5;
  const int iNonlinCorrectStart = 6;
  const int iNonlinCorrectLen   = 8;
  double    lfNonlinCorrectCoeff [8];  
  double    lfWaveLenCalibCoeff  [4];  
  double    fStrayLightConstant = -1;  
  for (int iConfigVar = 0; iConfigVar < LIBOOPT::getNumConfigVars(); ++iConfigVar)
  {
    char  lbAnswer[20];
    int   iError = LIBOOPT::queryConfig(fd(), iConfigVar, lbAnswer);
    if (iError != 0)
    {
      printf("OceanOpticsServer::configure(): queryConfig(%d) failed: %d\n", iConfigVar, iError);
      return 1;
    }
    lbAnswer[19] = 0;
    
    if ( iConfigVar >= iWaveLenCalibStart && iConfigVar < iWaveLenCalibStart+iWaveLenCalibLen)
    {    
        lfWaveLenCalibCoeff[iConfigVar - iWaveLenCalibStart] = strtod(lbAnswer, NULL);
      if (_iDebugLevel >= 1)    
        printf("  [%02d] %s\n    = %s (%lg)\n", iConfigVar, LIBOOPT::getConfigVarName(iConfigVar), lbAnswer, lfWaveLenCalibCoeff[iConfigVar - iWaveLenCalibStart]);        
    }        
    else if ( iConfigVar >= iNonlinCorrectStart && iConfigVar < iNonlinCorrectStart+iNonlinCorrectLen)
    {
      lfNonlinCorrectCoeff[iConfigVar - iNonlinCorrectStart] = strtod(lbAnswer, NULL);
      if (_iDebugLevel >= 1)    
        printf("  [%02d] %s\n    = %s (%lg)\n", iConfigVar, LIBOOPT::getConfigVarName(iConfigVar), lbAnswer, lfNonlinCorrectCoeff[iConfigVar - iNonlinCorrectStart]);        
    }
    else if ( iConfigVar == iftrayLightConstant )
    {
      fStrayLightConstant = strtod(lbAnswer, NULL);
      if (_iDebugLevel >= 1)
        printf("  [%02d] %s\n    = %s (%lg)\n", iConfigVar, LIBOOPT::getConfigVarName(iConfigVar), lbAnswer, fStrayLightConstant);
    }        
    else
    {
      if (_iDebugLevel >= 1)    
        printf("  [%02d] %s\n    = %s\n", iConfigVar, LIBOOPT::getConfigVarName(iConfigVar), lbAnswer);              
    }
  }  

  
  new (&config) OceanOpticsConfigType(config.exposureTime(), lfWaveLenCalibCoeff, fStrayLightConstant, lfNonlinCorrectCoeff);

  /*
   * start to configure the device 
   */  
  int iError = LIBOOPT::clearDataBuffer(fd());
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): clearDataBuffer() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }

  double fIntegrationTimeInUs = config.exposureTime() * 1e6;
  iError = LIBOOPT::setIntegrationTime(fd(), fIntegrationTimeInUs);
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): setIntegrationTime() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }
  
  iError = LIBOOPT::setTriggerMode(fd(), LIBOOPT::TRIGGER_MODE_EXT_HW);
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): setTriggerMode() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }            

  bool    bReadOk;
  double  fTemperatureInC;
  iError = LIBOOPT::getTemperature(fd(), bReadOk, fTemperatureInC);
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): getTemperature() failed: %d\n", iError);
    return 1;
  }
  printf("Reading Temperature: Read status %s. Value = %.2lf C\n", (bReadOk? "OK" : "Fail"), fTemperatureInC);
  
  printf("Reading registers:\n");  
  for (int iRegister = 0; iRegister < LIBOOPT::getNumRegisters(); ++iRegister)
  {
    int iAddr = LIBOOPT::getRegisterAddr(iRegister);

    uint16_t u16RegVal;
    int   iError = LIBOOPT::readRegister(fd(), iAddr, u16RegVal);
    if (iError != 0)
    {
      printf("OceanOpticsServer::configure(): readRegister(%d) failed: %d\n", iRegister, iError);
      return ERR_LIBOOPT_FAIL;
    }

    printf("  [%02d] <0x%02x> %s\n    = %d (0x%04x)\n", iRegister, iAddr, LIBOOPT::getRegisterName(iRegister), u16RegVal, u16RegVal);
  }
  
  LIBOOPT::SpecStatus status;
  if (LIBOOPT::queryStatus(fd(), status) != 0)
  {
    printf("OceanOpticsServer::configure(): queryStatus() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }
  printf("Device Status:\n");
  LIBOOPT::printStatus(status);       
  
  iError = LIBOOPT::arm(fd(), true, 0);
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): arm() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }     

  iError = LIBOOPT::setPollEnable(fd(), 1);
  if (iError != 0)
  {
    printf("OceanOpticsServer::configure(): setPollEnable() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }     
  
  _count = 0;
  initSpectraCheck();
  return 0;
}

unsigned OceanOpticsServer::unconfigure()
{   
  if (!devicePresent())
    return ERR_DEVICE_NOT_PRESENT;
    
  int iError = LIBOOPT::setPollEnable(fd(), 0);
  if (iError != 0)
  {
    printf("OceanOpticsServer::unconfigure(): setPollEnable() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }         
  
  uint64_t u64NumTotalFrames, u64NumDelayedFrames, u64NumDiscardFrames;
  iError = LIBOOPT::unarm(fd(), u64NumTotalFrames, u64NumDelayedFrames, u64NumDiscardFrames);
  if (iError != 0)
  {
    printf("OceanOpticsServer::unconfigure(): unarm() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }
  
  if (_iDebugLevel >= 2 && u64NumTotalFrames > 0)  
    finalSpectraReport();
    
  printf("Count %Lu Captured frames %Lu delayed %Lu discarded %Lu\n", (long long unsigned) _count, (long long unsigned) u64NumTotalFrames, (long long unsigned) u64NumDelayedFrames, (long long unsigned) u64NumDiscardFrames);
  
  iError = LIBOOPT::clearDataBuffer(fd());
  if (iError != 0)
  {
    printf("OceanOpticsServer::unconfigure(): clearDataBuffer() failed: %d\n", iError);
    return ERR_LIBOOPT_FAIL;
  }
  
  return 0;
}

int OceanOpticsServer::fetch( char* payload, int flags )
{
  if (!devicePresent())
    return 0;
    
  OceanOpticsDataType data;
  
  OceanOpticsDataType* pData = (OceanOpticsDataType*) (payload + sizeof(_xtc));
  int iNumBytesRead = read(fd(), pData, OceanOpticsDataType::iDataReadSize);    
  if (iNumBytesRead != OceanOpticsDataType::iDataReadSize)
  {
    printf("OceanOpticsServer::fetch(): read() failed: %d %s\n", iNumBytesRead, strerror(errno));
    return ERR_READ_FAIL;
  }

  *(Xtc*) payload = _xtc; // setup the Xtc header
  _count = pData->frameCounter(); // setup the counter for event building matching
  
  if (_iDebugLevel >= 2)
    checkSpectraInfo(pData);
     
   return sizeof(_xtc) + sizeof(OceanOpticsDataType);
}

unsigned OceanOpticsServer::count() const
{
   return _count;
}

} // namespace pds

static int initSpectraCheck()
{
  int iPixelDataSize, iNumTotalPixels;
  LIBOOPT::getSpectraDataInfo(iPixelDataSize, iNumTotalPixels, iPostheaderOffset);
  
  clock_gettime(CLOCK_REALTIME, &tsPrevReport);
  tsPrevFetch = tsPrevReport;
  
  histTimeFrame.reset();
  histFetchInterval.reset();
  histTimeStart.reset();
  histTimeInterval.reset();
  histTimeFirstData.reset();
  histTimeGetData.reset();
  histTimeGap.reset();
  histNumSpectraInQueue.reset();
  histLongIntervalShot.reset();
  
  return 0;
}

static int checkSpectraInfo(OceanOpticsDataType* pData)
{
  LIBOOPT::SpectraPostheader* pSpectraPostheader = (LIBOOPT::SpectraPostheader*) ((char*)pData + iPostheaderOffset);
    
  double  fTimeFrame      = pSpectraPostheader->tsTimeFrameEnd.tv_sec - pSpectraPostheader->tsTimeFrameInit.tv_sec + pSpectraPostheader->tsTimeFrameEnd.tv_nsec * 1e-9 - pSpectraPostheader->tsTimeFrameInit.tv_nsec * 1e-9;
  double  fTimeFrameStart = pSpectraPostheader->tsTimeFrameStart.tv_sec - pSpectraPostheader->tsTimeFrameInit.tv_sec + pSpectraPostheader->tsTimeFrameStart.tv_nsec * 1e-9 - pSpectraPostheader->tsTimeFrameInit.tv_nsec * 1e-9;
  double  fTimeFirstData  = pSpectraPostheader->tsTimeFrameFirstData.tv_sec - pSpectraPostheader->tsTimeFrameInit.tv_sec + pSpectraPostheader->tsTimeFrameFirstData.tv_nsec * 1e-9 - pSpectraPostheader->tsTimeFrameInit.tv_nsec * 1e-9;
  double  fTimeGetData    = pSpectraPostheader->tsTimeFrameEnd.tv_sec - pSpectraPostheader->tsTimeFrameFirstData.tv_sec + pSpectraPostheader->tsTimeFrameEnd.tv_nsec * 1e-9 - pSpectraPostheader->tsTimeFrameFirstData.tv_nsec * 1e-9;
  double  fTimeInterval   = pSpectraPostheader->tsTimeFrameInit.tv_sec - tsPrevInit.tv_sec + pSpectraPostheader->tsTimeFrameInit.tv_nsec * 1e-9 - tsPrevInit.tv_nsec * 1e-9;
  double  fTimeGap        = tsPrevEnd.tv_sec - pSpectraPostheader->tsTimeFrameInit.tv_sec + (tsPrevEnd.tv_nsec - pSpectraPostheader->tsTimeFrameInit.tv_nsec) * 1e-9;

  tsPrevInit = pSpectraPostheader->tsTimeFrameInit;
  tsPrevEnd  = pSpectraPostheader->tsTimeFrameEnd;
  
  struct timespec tsCur;
  clock_gettime(CLOCK_REALTIME, &tsCur);
  
  double  fFetchInterval      = (tsCur.tv_sec - tsPrevFetch.tv_sec) + (tsCur.tv_nsec - tsPrevFetch.tv_nsec) * 1e-9;  
  tsPrevFetch = tsCur;
  
  double  fLongIntervalBound;
  if (pSpectraPostheader->u64FrameCounter == 0)
  {
    fTimeInterval       = fTimeGap = 0;
    fLongIntervalBound  = fFetchInterval * fLongIntervalRatio;
  }
  else
  {
    histTimeFrame     .addValue(fTimeFrame * 1e3);
    histTimeStart     .addValue(fTimeFrameStart * 1e3);
    histTimeFirstData .addValue(fTimeFirstData * 1e3);
    histTimeGetData   .addValue(fTimeGetData * 1e3);
    histFetchInterval .addValue(fFetchInterval * 1e3);
    
    histTimeInterval.addValue(fTimeInterval * 1e3);
    histTimeGap.addValue(fTimeGap * 1e3);
    
    fLongIntervalBound  = (histFetchInterval.avgInRange() * fLongIntervalRatio) * 1e-3;
  }  
  
  bool    bLongIntervalReport = fTimeFirstData >= fLongIntervalBound;
  if (bLongIntervalReport)
  {
    ++iNumLongInterval;
    histLongIntervalShot.addValue(pSpectraPostheader->u64FrameCounter);
    //histLongIntervalShot.report("Long Interval Shot");
  }    
  
  bool bRegularReport = (tsCur.tv_sec >= tsPrevReport.tv_sec + iReportInterval);
  
  int iNumSpectraInQueue = pSpectraPostheader->dataInfo.i8NumSpectraInQueue;
  histNumSpectraInQueue.addValue(iNumSpectraInQueue);
  
  if (bLongIntervalReport || bRegularReport || iNumSpectraInQueue > 0)
  {
    tsPrevReport = tsCur;  
    
    if (bLongIntervalReport)
      printf("\n***");
    else
      printf("\n");
      
    printf("Frame %Lu (Long Interval %d)\n", (long long unsigned) pSpectraPostheader->u64FrameCounter, iNumLongInterval);
      
    printf("  Frame time %.3f fetch %.3f bound %.3f interval %.3f start %.3f first data %.3f get data %.3f gap %.3f\n",
      fTimeFrame * 1e3, fFetchInterval * 1e3, fLongIntervalBound * 1e3, fTimeInterval * 1e3, fTimeFrameStart * 1e3, fTimeFirstData * 1e3, fTimeGetData * 1e3, fTimeGap * 1e3);

    if (iNumSpectraInQueue > 0)
      printf("*** Frame %Lu  Spectra in queue %d\n", (long long unsigned) pSpectraPostheader->u64FrameCounter, pSpectraPostheader->dataInfo.i8NumSpectraInQueue);        
      
    printSpectraHeaderInfo(pSpectraPostheader);
      
    histTimeFrame.report("Frame Time");
    histFetchInterval.report("Fetch Interval");
    histTimeStart.report("Frame Start");
    histTimeInterval.report("Frame Interval");
    histTimeFirstData.report("Frame First Data Time");
    histTimeGetData.report("Frame Get Data Time");
    histTimeGap.report("Frame Gap");
    histNumSpectraInQueue.report("Spectra In Queue");
    printf("\n");
  }
      
  return 0;
}

static int finalSpectraReport()
{
  histTimeFrame.report("Frame Time");
  histFetchInterval.report("Fetch Interval");
  histTimeStart.report("Frame Start");
  histTimeInterval.report("Frame Interval");
  histTimeFirstData.report("Frame First Data Time");
  histTimeGetData.report("Frame Get Data Time");
  histTimeGap.report("Frame Gap");
  histNumSpectraInQueue.report("Spectra In Queue");
  histLongIntervalShot.report("Long Interval Shot");  
  
  return 0;
}

static int getLocalTime( const LIBOOPT::timespec64& ts, char* sTime )
{
  static const char timeFormatStr[40] = "%04Y-%02m-%02d %02H:%02M:%02S"; /* Time format string */
  static char sTimeText[64];

  struct tm tmTimeStamp;
  time_t    tOrg = ts.tv_sec;
  localtime_r( (const time_t*) (void*) &tOrg, &tmTimeStamp );
  strftime(sTimeText, sizeof(sTimeText), timeFormatStr, &tmTimeStamp );

  strncpy(sTime, sTimeText, sizeof(sTimeText));
  return 0;
}

static int printSpectraHeaderInfo(LIBOOPT::SpectraPostheader* pSpectraPostheader)
{
  char sTimeFrameInit     [64];
  char sTimeFrameStart    [64];
  char sTimeFrameFirstData[64];
  char sTimeFrameEnd      [64];

  getLocalTime(pSpectraPostheader->tsTimeFrameInit, sTimeFrameInit);
  getLocalTime(pSpectraPostheader->tsTimeFrameStart, sTimeFrameStart);
  getLocalTime(pSpectraPostheader->tsTimeFrameFirstData, sTimeFrameFirstData);
  getLocalTime(pSpectraPostheader->tsTimeFrameEnd, sTimeFrameEnd);

  printf(
    "    u64FrameCounter      %Lu\n"
    "    u64NumDelayedFrames  %Lu\n"
    "    u64NumDiscardFrames  %Lu\n"
    "    tsTimeFrameInit      %s.%d\n"
    "    tsTimeFrameStart     %s.%d\n"
    "    tsTimeFrameFirstData %s.%d\n"
    "    tsTimeFrameEnd       %s.%d\n",
    (long long unsigned) pSpectraPostheader->u64FrameCounter,
    (long long unsigned) pSpectraPostheader->u64NumDelayedFrames,
    (long long unsigned) pSpectraPostheader->u64NumDiscardFrames,
    sTimeFrameInit, (int) pSpectraPostheader->tsTimeFrameInit.tv_nsec,
    sTimeFrameStart, (int) pSpectraPostheader->tsTimeFrameStart.tv_nsec,
    sTimeFrameFirstData, (int) pSpectraPostheader->tsTimeFrameFirstData.tv_nsec,
    sTimeFrameEnd, (int) pSpectraPostheader->tsTimeFrameEnd.tv_nsec
    );

  printf(
    "    i32Version           0x%08x\n"
    "    i8NumSpectraInData   %d\n"
    "    i8NumSpectraInQueue  %d\n"
    "    i8NumSpectraUnused   %d\n",
    pSpectraPostheader->dataInfo.i32Version,
    pSpectraPostheader->dataInfo.i8NumSpectraInData,
    pSpectraPostheader->dataInfo.i8NumSpectraInQueue,
    pSpectraPostheader->dataInfo.i8NumSpectraUnused
    );

  return 0;
}

