#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "liboopt.hh"

#define OOPT_IOCTL_MAGIC      'k' // 0x6b , not used by official drivers
#define OOPT_IOCTL_BASE       128
#define OOPT_IOCTL_CMD_NORET  _IOW ( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 0, IoctlInfo )
#define OOPT_IOCTL_CMD_RET    _IOWR( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 1, IoctlInfo )
#define OOPT_IOCTL_ARM        _IOWR( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 2, ArmInfo )
#define OOPT_IOCTL_UNARM      _IOWR( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 3, ArmInfo )
#define OOPT_IOCTL_POLLENABLE _IOWR( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 4, PollInfo )
#define OOPT_IOCTL_DEVICEINFO _IOWR( OOPT_IOCTL_MAGIC, OOPT_IOCTL_BASE + 5, QueryDeviceInfo )

namespace LIBOOPT
{

typedef enum
{
  LIMIT_NONE = 0,
  LIMIT_NUM  = 1,
  LIMIT_MAX  = 2,
} ArmLimitType;

#pragma pack(4)
#define OOPT_CMD_BULK_SIZE    64

typedef struct tagIoctlInfo
{
  uint8_t   u8LenCommand;
  uint8_t   u8LenResult;
  uint16_t  u16CmdTimeoutInMs;
  uint16_t  u16RetTimeoutInMs;
  uint8_t   lu8Command[OOPT_CMD_BULK_SIZE];
  uint8_t   lu8Result [OOPT_CMD_BULK_SIZE];
} IoctlInfo;

typedef struct
{
  int       iArmLimitType;
  uint64_t  u64NumTotalFrames;
  uint64_t  u64NumDelayedFrames;
  uint64_t  u64NumDiscardFrames;
} ArmInfo;

typedef struct
{
  int       iPollEnable;
} PollInfo;

typedef struct
{
  int       iProductId;
} QueryDeviceInfo;

#pragma pack()

const int iDefaultCmdTimeoutInMs = 500;
const int iDefaultRetTimeoutInMs = 500;

static void sleepInSeconds(double fSleep)
{
  timeval timeSleepMicro = {0, (int) (fSleep * 1e6)};
  // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
  select( 0, NULL, NULL, NULL, &timeSleepMicro);
}

static int getDeviceType(int fd, int& iDevType);

// device type: 0 - HR4000, 1 - USB2000P
int initDevice(int fd, int& iDeviceType)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 1;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  ioctlInfo.lu8Command[0]     = 0x1;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::initDevice(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.0001); // 100 us

  iError = getDeviceType(fd, iDeviceType);
  if (iError != 0)
    printf("liboopt::initDevice(): getDeviceType() failed, error = %d\n", iError);

  return iError;
}

int queryConfig   (int fd, int iConfigVar, char lbConfigVal[OOPT_CONFIGVAR_RETLEN])
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 2;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]     = 0x5;
  ioctlInfo.lu8Command[1]     = iConfigVar;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_RET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::queryConfig(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  if (ioctlInfo.u8LenResult != OOPT_CONFIGVAR_RETLEN + 2)
  {
    printf("liboopt::queryConfig(): return length (%d) != %d\n", ioctlInfo.u8LenResult, OOPT_CONFIGVAR_RETLEN+2);
    return ERROR_INVALID_DATA;
  }

  if (ioctlInfo.lu8Result[0] != ioctlInfo.lu8Command[0])
  {
    printf("liboopt::queryConfig(): return value[0] (0x%x) != query cmd (0x%x)\n", ioctlInfo.lu8Result[0], ioctlInfo.lu8Command[0]);
    return ERROR_INVALID_DATA;
  }

  if (ioctlInfo.lu8Result[1] != iConfigVar)
  {
    printf("liboopt::queryConfig(): return value[1] (%d) != config index (%d)\n", ioctlInfo.lu8Result[1], iConfigVar);
    return ERROR_INVALID_DATA;
  }

  memcpy(lbConfigVal, ioctlInfo.lu8Result + 2, OOPT_CONFIGVAR_RETLEN);
  return 0;
}

int setConfig(int fd, int iConfigVar, const char lbConfigVal[OOPT_CONFIGVAR_RETLEN])
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 2 + OOPT_CONFIGVAR_RETLEN;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]     = 0x6;
  ioctlInfo.lu8Command[1]     = iConfigVar;
  memcpy(ioctlInfo.lu8Command + 2, lbConfigVal, OOPT_CONFIGVAR_RETLEN);

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::setConfig(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.0001); // 100 us
  return iError;
}

int readRegister(int fd, int iRegisterAddr, uint16_t& u16RegisterVal)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 2;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]     = 0x6B;
  ioctlInfo.lu8Command[1]     = iRegisterAddr;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_RET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::readRegister(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  const int iReadRegisterRetLen = 3;
  if (ioctlInfo.u8LenResult != iReadRegisterRetLen)
  {
    printf("liboopt::readRegister(): return length (%d) != %d\n", ioctlInfo.u8LenResult, iReadRegisterRetLen);
    return ERROR_INVALID_DATA;
  }

  if (ioctlInfo.lu8Result[0] != iRegisterAddr)
  {
    printf("liboopt::readRegister(): return value[0] (0x%x) != register addr (0x%x)\n", ioctlInfo.lu8Result[0], iRegisterAddr);
    return ERROR_INVALID_DATA;
  }

  u16RegisterVal = * (uint16_t*) (&ioctlInfo.lu8Result[1]);
  return 0;
}

int setRegister(int fd, int iRegisterAddr, uint16_t u16RegisterVal)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 4;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]               = 0x6A;
  ioctlInfo.lu8Command[1]               = iRegisterAddr;
  *(uint16_t*) &ioctlInfo.lu8Command[2]  = u16RegisterVal;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::setRegister(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.0001); // 100 us
  return iError;
}

int getTemperature(int fd, bool& bReadOk, double& fTemperatureInC)
{
  bReadOk = false;

  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 1;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]     = 0x6C;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_RET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::getTemperature(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  const int iGetTemperatureRetLen = 3;
  if (ioctlInfo.u8LenResult != iGetTemperatureRetLen)
  {
    printf("liboopt::getTemperature(): return length (%d) != %d\n", ioctlInfo.u8LenResult, iGetTemperatureRetLen);
    return ERROR_INVALID_DATA;
  }

  const int iReadOk = 8;
  if (ioctlInfo.lu8Result[0] == iReadOk)
    bReadOk = true;

  uint16_t      uTemperature         = *(uint16_t*) &ioctlInfo.lu8Result[1];
  const double  fTemperatureMultiply = 0.003906;

  fTemperatureInC  = uTemperature * fTemperatureMultiply;
  return 0;
}

int setIntegrationTime(int fd, double fIntegrationTimeInUs)
{
  uint32_t uIntegrationTime = (int) (fIntegrationTimeInUs);

  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 5;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]               = 0x02;
  *(uint32_t*) &ioctlInfo.lu8Command[1] = uIntegrationTime;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::setIntegrationTime(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.0001); // 100 us
  return iError;
}

int setTriggerMode(int fd, TriggerMode eTriggerMode)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 3;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]               = 0x0A;
  *(uint16_t*) &ioctlInfo.lu8Command[1] = (int) eTriggerMode;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::setTriggerMode(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.0001); // 100 us
  return iError;
}

int setPowerMode(int fd, PowerMode ePowerMode)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 3;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]               = 0x04;
  *(uint16_t*) &ioctlInfo.lu8Command[1] = (int) ePowerMode;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_NORET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::setPowerMode(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  sleepInSeconds(0.1); // 0.1 second
  return iError;
}


int queryStatus(int fd, SpecStatus& status)
{
  IoctlInfo ioctlInfo;
  ioctlInfo.u8LenCommand      = 1;
  ioctlInfo.u8LenResult       = 0;
  ioctlInfo.u16CmdTimeoutInMs = iDefaultCmdTimeoutInMs;
  ioctlInfo.u16RetTimeoutInMs = iDefaultRetTimeoutInMs;

  memset(ioctlInfo.lu8Command, 0, sizeof(ioctlInfo.lu8Command));
  memset(ioctlInfo.lu8Result , 0, sizeof(ioctlInfo.lu8Result));
  ioctlInfo.lu8Command[0]     = 0xFE;

  int iError = ioctl(fd, OOPT_IOCTL_CMD_RET, &ioctlInfo);
  if (iError != 0)
  {
    printf("liboopt::queryStatus(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  const int iQueryStatusRetLen = 16;
  if (ioctlInfo.u8LenResult != iQueryStatusRetLen)
  {
    printf("liboopt::queryStatus(): return length (%d) != %d\n", ioctlInfo.u8LenResult, iQueryStatusRetLen);
    return ERROR_INVALID_DATA;
  }

  status.uNumPixels       = *((uint16_t*)&ioctlInfo.lu8Result[0]);
  status.uIntegrationTimeInUs
                          = *((uint32_t*)&ioctlInfo.lu8Result[2]);
  status.uLampEnable      = ioctlInfo.lu8Result[6];
  status.uTriggerMode     = ioctlInfo.lu8Result[7];
  status.uSpectrumAcqStat = ioctlInfo.lu8Result[8];
  status.uPacketsInSpectra= ioctlInfo.lu8Result[9];
  status.uPowerDownFlag   = ioctlInfo.lu8Result[10];
  status.uPacketCount     = ioctlInfo.lu8Result[11];
  status.uReserved1       = ioctlInfo.lu8Result[12];
  status.uReserved2       = ioctlInfo.lu8Result[13];
  status.uUsbSpeed        = ioctlInfo.lu8Result[14];
  status.uReserved3       = ioctlInfo.lu8Result[15];

  return 0;
}

int arm(int fd, bool bKeepRunning, int iNumFrames)
{
  ArmInfo armInfo;

  if ( bKeepRunning )
  {
    armInfo.iArmLimitType     = (int) LIMIT_NONE;
    armInfo.u64NumTotalFrames = 0;
  }
  else
  {
    armInfo.iArmLimitType     = (int) LIMIT_NUM;
    armInfo.u64NumTotalFrames = iNumFrames;
  }

  int iError = ioctl(fd, OOPT_IOCTL_ARM, &armInfo);
  if (iError != 0)
  {
    printf("liboopt::arm(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  return 0;
}

int unarm(int fd, uint64_t& u64NumTotalFrames, uint64_t& u64NumDelayedFrames, uint64_t& u64NumDisacrdFrames)
{
  ArmInfo armInfo;

  int iError = ioctl(fd, OOPT_IOCTL_UNARM, &armInfo);
  if (iError != 0)
  {
    printf("liboopt::unarm(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  u64NumTotalFrames   = armInfo.u64NumTotalFrames;
  u64NumDelayedFrames = armInfo.u64NumDelayedFrames;
  u64NumDisacrdFrames = armInfo.u64NumDiscardFrames;
  return 0;
}

int setPollEnable(int fd, int iEnable)
{
  PollInfo pollInfo;
  pollInfo.iPollEnable = iEnable;

  int iError = ioctl(fd, OOPT_IOCTL_POLLENABLE, &pollInfo);
  if (iError != 0)
  {
    printf("liboopt::setPollEnable(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  return 0;
}

int getDeviceType(int fd, int& iDevType)
{
  QueryDeviceInfo queryDeviceInfo = {0};

  int iError = ioctl(fd, OOPT_IOCTL_DEVICEINFO, &queryDeviceInfo);
  if (iError != 0)
  {
    printf("liboopt::getDeviceType(): ioctl() failed, error = %d, %s\n", iError, strerror(errno));
    return iError;
  }

  if (queryDeviceInfo.iProductId == 0x1012)
    iDevType = 0;
  else if (queryDeviceInfo.iProductId == 0x101E)
    iDevType =  1;
  else
  {
    printf("liboopt::getDeviceType(): Unsupported device, product id = 0x%x\n", queryDeviceInfo.iProductId);
    return 1;
  }

  return 0;
}

int clearDataBuffer(int fd)
{
  SpecStatus status;
  int iError = queryStatus(fd, status);
  if (iError != 0)
  {
    printf("liboopt::clearDataBuffer(): queryStatus() failed: %d\n", iError);
    return ERROR_LIB_FUNC;
  }

  iError = setTriggerMode(fd, TRIGGER_MODE_CONTINUOUS);
  if (iError != 0)
  {
    printf("liboopt::clearDataBuffer(): setTriggerMode() failed: %d\n", iError);
    return ERROR_LIB_FUNC;
  }

  const unsigned int uIntegrationTimeInUs = 1000;
  iError = setIntegrationTime(fd, uIntegrationTimeInUs);
  if (iError != 0)
  {
    printf("liboopt::clearDataBuffer(): setIntegrationTime() failed: %d\n", iError);
    return ERROR_LIB_FUNC;
  }

  iError = arm(fd, false, 1);
  if (iError != 0)
  {
    printf("liboopt::clearDataBuffer(): arm() failed: %d\n", iError);
    return ERROR_LIB_FUNC;
  }

  sleepInSeconds(0.01);

  uint64_t u64NumTotalFrames, u64NumDelayedFrames, u64NumDiscardFrames;
  iError = unarm(fd, u64NumTotalFrames, u64NumDelayedFrames, u64NumDiscardFrames);
  if (iError != 0)
  {
    printf("liboopt::clearDataBuffer(): unarm() failed: %d\n", iError);
    return ERROR_LIB_FUNC;
  }

  if ( (TriggerMode) status.uTriggerMode != TRIGGER_MODE_CONTINUOUS )
  {
    iError = setTriggerMode(fd, (TriggerMode)status.uTriggerMode);
    if (iError != 0)
    {
      printf("liboopt::clearDataBuffer(): setTriggerMode() failed: %d\n", iError);
      return ERROR_LIB_FUNC;
    }
  }

  if ( status.uIntegrationTimeInUs != uIntegrationTimeInUs )
  {
    iError = setIntegrationTime(fd, status.uIntegrationTimeInUs);
    if (iError != 0)
    {
      printf("liboopt::clearDataBuffer(): setIntegrationTime() failed: %d\n", iError);
      return ERROR_LIB_FUNC;
    }
  }

  return 0;
}

int printStatus(const SpecStatus& status)
{
  printf("  # Pixels              : 0x%04x (%d)\n", status.uNumPixels, status.uNumPixels);
  printf("  Integration Time (us) : 0x%04x (%d)\n", status.uIntegrationTimeInUs, status.uIntegrationTimeInUs);
  printf("  Lamp Enable (0-L 1-H) : 0x%02x (%d)\n", status.uLampEnable, status.uLampEnable);
  printf("  Trigger Mode\n"
         "    0-Continu  1-Sof+Trg\n"
         "    2-Ext Sync 3-Ext HW : 0x%02x (%d)\n", status.uTriggerMode, status.uTriggerMode);
  printf("  Spectral Acq state    : 0x%02x (%d)\n", status.uSpectrumAcqStat, status.uSpectrumAcqStat);
  printf("  Packets in Spectral   : 0x%02x (%d)\n", status.uPacketsInSpectra, status.uPacketsInSpectra);
  printf("  Power (0-Up 1-Down)   : 0x%02x (%d)\n", status.uPowerDownFlag, status.uPowerDownFlag);
  printf("  Packet Count in EP    : 0x%02x (%d)\n", status.uPacketCount, status.uPacketCount);
  printf("  USB Speed (0-F 128-H) : 0x%02x (%d)\n", status.uUsbSpeed, status.uUsbSpeed);

  return 0;
}

const int   iNumConfigVars = 20;
const int   getNumConfigVars()
{
  return iNumConfigVars;
}

const char* getConfigVarName(int iConfigVar)
{
  const char* lsConfigVarName[iNumConfigVars] =
  {
    "0  Serial Number",
    "1  0th order Wavelength Calibration Coefficient",
    "2  1st order Wavelength Calibration Coefficient",
    "3  2nd order Wavelength Calibration Coefficient",
    "4  3rd order Wavelength Calibration Coefficient",
    "5  Stray light constant",
    "6  0th order non-linearity correction coefficient",
    "7  1st order non-linearity correction coefficient",
    "8  2nd order non-linearity correction coefficient",
    "9  3rd order non-linearity correction coefficient",
    "10  4th order non-linearity correction coefficient",
    "11  5th order non-linearity correction coefficient",
    "12  6th order non-linearity correction coefficient",
    "13  7th order non-linearity correction coefficient",
    "14  Polynomial order of non-linearity calibration",
    "15  Optical bench configuration: gg fff sss\n"
    "       gg  Grating #, fff  filter wavelength, sss  slit size",
    "16  HR4000 configuration: AWL V\n"
    "       A  Array coating Mfg, W  Array wavelength (VIS, UV, OFLV),\n"
    "       L  L2 lens installed, V  CPLD Version",
    "17  Reserved",
    "18  Reserved",
    "19  Reserved"
  };

  if (iConfigVar < 0 || iConfigVar >= iNumConfigVars)
    return "Config Index Out of Range";

  return lsConfigVarName[iConfigVar];
}

const int iNumRegisters = 17;
int getNumRegisters()
{
  return iNumRegisters;
}

const struct { int iAddr; char* sRegisterName; } lRegisterInfo[] =
{
  {0x00, "*Set Master Clock Counter Divisor. Master Clock freq = 48MHz/Divisor.\n"
         "  Def: 12"},
  {0x04, "FPGA Firmware Version (Read Only)"},
  {0x08, "Continuous Strobe Timer Interval.\n"
         "  Def: 10 (100Hz)"},
  {0x0C, "Continuous Strobe Base Clock. Divisor operates from 48MHz Clock.\n"
         "  Def: 48000 (1KHz)"},
  {0x10, "*Integration Period Base Clock Base Freq = 48MHz.\n"
         "  Def: 480 (10us)"},
  {0x18, "*Integration Clock Timer. Def: 600 (6ms)"},
  {0x20, "Shutter Clock (contact OOI for info)"},
  {0x28, "Hardware Trigger Delay  Number of Master Clock cycles\n"
         "  to delay when in External Hardware Trigger mode before\n"
         "  the start of the integration period"},
  {0x2C, "*Trigger Mode 1 = External Synchronization 2 = External Hardware Trigger"},
  {0x30, "Reserved"},
  {0x38, "Single Strobe High Clock Transition. Def: 1"},
  {0x3C, "Single Strobe Low Clock Transition. Def: 10"},
  {0x40, "Strobe Enable"},
  {0x48, "GPIO Mux Register (0 = pin is GPIO pin, 1 = pin is alternate function)"},
  {0x50, "GPIO Output Enable (1 = pin is output, 0= pin is input)"},
  {0x54, "GPIO Data Register\n"
         "  For Ouput = Write value of signal\n"
         "  For Input = Read current GPIO state"},
  {0x58, "Reserved"},
};

int         getRegisterAddr(int iIndex)
{
  return lRegisterInfo[iIndex].iAddr;
}

const char* getRegisterName(int iIndex)
{
  return lRegisterInfo[iIndex].sRegisterName;
}

int getSpectraDataInfo(int iDeviceType, int& iFrameDataSize, int &iNumTotalPixels, int& iPostheaderOffset)
{
  switch (iDeviceType)
  {
  case 0: // HR4000
    iFrameDataSize    = 8192;
    iNumTotalPixels   = 3840;
    break;
  case 1:
    iFrameDataSize    = 4608;
    iNumTotalPixels   = 2048;
    break;
  }

  const int iDataBulkSize = 512;
  iPostheaderOffset = iFrameDataSize - iDataBulkSize;
  return 0;
}

} // namespace LIBOOPT
