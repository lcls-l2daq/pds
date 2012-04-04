#ifndef LIBOOPT_H
#define LIBOOPT_H

#include <stdint.h>

namespace LIBOOPT
{

#define OOPT_CONFIGVAR_RETLEN 15

int initHR4000        (int fd);
int queryConfig       (int fd, int iConfigVar, char lbConfigVal[OOPT_CONFIGVAR_RETLEN]);
int setConfig         (int fd, int iConfigVar, const char lbConfigVal[OOPT_CONFIGVAR_RETLEN]);
int readRegister      (int fd, int iRegisterAddr, uint16_t& u16RegisterVal);
int setRegister       (int fd, int iRegisterAddr, uint16_t u16RegisterVal);
int getTemperature    (int fd, bool& bReadOk, double& fTemperatureInC);
int setIntegrationTime(int fd, double fIntegrationTimeInUs);
int arm               (int fd, bool bKeepRunning, int iNumFrames);
int unarm             (int fd, uint64_t& u64NumTotalFrames, uint64_t& u64NumDelayedFrames, uint64_t& u64NumDisacrdFrames);
int setPollEnable     (int fd, int iEnable);

const int   getNumConfigVars();
const char* getConfigVarName(int iConfigVar);
int         getNumRegisters();
int         getRegisterAddr(int iIndex);
const char* getRegisterName(int iIndex);

typedef struct
{
  uint16_t uNumPixels;
  uint32_t uIntegrationTimeInUs;
  uint8_t  uLampEnable;
  uint8_t  uTriggerMode;
  uint8_t  uSpectrumAcqStat ;
  uint8_t  uPacketsInSpectra;
  uint8_t  uPowerDownFlag;
  uint8_t  uPacketCount;
  uint8_t  uReserved1;
  uint8_t  uReserved2;
  uint8_t  uUsbSpeed;
  uint8_t  uReserved3;
} SpecStatus;

int queryStatus(int fd, SpecStatus& status);
int printStatus(const SpecStatus& status);

enum TriggerMode
{
  TRIGGER_MODE_CONTINUOUS   = 0,
  TRIGGER_MODE_SOFT_ENABLE  = 1,
  TRIGGER_MODE_EXT_SYNC     = 2,
  TRIGGER_MODE_EXT_HW       = 3,
};
int setTriggerMode    (int fd, TriggerMode eTriggerMode);

enum PowerMode
{
  POWER_MODE_UP   = 0,
  POWER_MODE_DOWN = 1,
};
int setPowerMode      (int fd, PowerMode ePowerMode);

#pragma pack(4)

typedef struct
{
  int32_t i32Version;
  int8_t  i8NumSpectraInData;
  int8_t  i8NumSpectraInQueue;
  int8_t  i8NumSpectraUnused;
  int8_t  iReserved1;
} DataInfo;

typedef struct
{
  uint64_t tv_sec;
  uint64_t tv_nsec;
} timespec64;

typedef struct
{
  uint64_t          u64FrameCounter;    // count from 0
  uint64_t          u64NumDelayedFrames;
  uint64_t          u64NumDiscardFrames;
  timespec64        tsTimeFrameInit;
  timespec64        tsTimeFrameStart;
  timespec64        tsTimeFrameFirstData;
  timespec64        tsTimeFrameEnd;
  DataInfo          dataInfo;
} SpectraPostheader;

#pragma pack()

int getSpectraDataInfo(int& iFrameDataSize, int &iNumTotalPixels, int& iPostheaderOffset);
int clearDataBuffer   (int fd);

enum ErrorCode
{
  ERROR_INVALID_DATA  = -1000,
  ERROR_LIB_FUNC      = -1001,
};

}

#endif
