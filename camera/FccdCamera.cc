//! FccdCamera.cc
//! See FccdCamera.hh for a description of what the class can do
//!
//! Copyright 2010, SLAC
//! Author: caf@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/FccdCamera.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/camera/CameraDriver.hh"
#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <new>

#define RESET_COUNT 0x80000000

using namespace Pds;

FccdCamera::FccdCamera() :
  _inputConfig(0)
{
  LastCount = 0;
}

FccdCamera::~FccdCamera() {
}

void FccdCamera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const FccdConfigType*>(p);
  //  ConfigReset();
}  

int FccdCamera::camera_depth() const 
{ return Pds::FCCD::FccdConfigV2::Eight_bit; }

int    FccdCamera::camera_width         () const
{ return Pds::FCCD::FccdConfigV2::Column_Pixels; }

int    FccdCamera::camera_height        () const
{ return Pds::FCCD::FccdConfigV2::Row_Pixels; }

int    FccdCamera::camera_taps          () const
{ return 2; }

const FccdConfigType& FccdCamera::Config() const
{ return *reinterpret_cast<const FccdConfigType*>(_outputBuffer); }

const char* FccdCamera::camera_name() const
{
  return FCCD_NAME;
}

//
// rstrip - strip white space from the end of a string
//
static void rstrip(char *buf, int len, char drop1, char drop2)
{
  while (len > 0) {
    --len;
    if ((buf[len] == drop1) || (buf[len] == drop2) || isspace(buf[len])) {
      buf[len] = 0x0;
    } else {
      break;
    }
  }
}

//
// makeWaveCommand -
//
static int makeWaveCommand(int patNum, int clkPattern, char *buf)
{
  int rv = -1;

  if ((patNum >= 0) && (patNum <= 14)) {
    sprintf(buf, "0a:%02x:%02x:%02x", patNum,
            (clkPattern & 0xff00) >> 8, clkPattern & 0xff);
    rv = 0;   // success
  }
  return (rv);
}

//
// makeDACWriteCommand -
//
static int makeDACWriteCommand(int address, float volts, float beginVolts, float endVolts, char *buf)
{
  int dacValue;
  int rv = -1;

  if ((address >= 0) && (address <= 0x20))
    {
    if (beginVolts < endVolts) {
      if (volts >= endVolts) {
        // saturated at endVolts
        dacValue = 0xfff;
      } else if (volts <= beginVolts) {
        // saturated at beginVolts
        dacValue = 0x0;
      } else {
        // between beginVolts and endVolts, where endVolts is greater
        dacValue = (int) floor(0.5 + (0xfff * (volts - beginVolts) / (endVolts - beginVolts)));
      }
    } else {
      if (volts <= endVolts) {
        // saturated at endVolts
        dacValue = 0xfff;
      } else if (volts >= beginVolts) {
        // saturated at beginVolts
        dacValue = 0x0;
      } else {
        // between beginVolts and endVolts, where beginVolts is greater
        dacValue = (int) floor(0.5 + (0xfff * (beginVolts - volts) / (beginVolts - endVolts)));
      }
    }
    sprintf(buf, "05:%02x:%02x:%02x", address, (dacValue & 0xff00) >> 8, dacValue & 0xff);
    rv = 0; // success
    }
  return (rv);
}

#define SZRESPONSE_MAXLEN  64
#define SENDBUF_MAXLEN     40

int FccdCamera::SendFccdCommand(CameraDriver& driver,
				const char *cmd)
{
  unsigned int ubuf;
  int sendLen, ii, ret;
  char szResponse[SZRESPONSE_MAXLEN];
  char sendBuf[SENDBUF_MAXLEN];

  // format of command string is aa:bb:cc ...
  sendLen = (strlen(cmd)+1) / 3;
  for (ii = 0; (ii < sendLen) && (ii < SENDBUF_MAXLEN); ii++) {
    if (sscanf(cmd+(ii*3), "%2x", &ubuf) == 1) {
      sendBuf[ii] = (char)ubuf;
    } else {
      printf(">> %s: Error reading cmd \"%s\"\n", __FUNCTION__, cmd);
      return (-EINVAL);
    }
  }
  ret = driver.SendBinary(sendBuf, sendLen, szResponse, SZRESPONSE_MAXLEN);
  rstrip(szResponse, ret, eotRead(), 'z');
  printf(">> %s: cmd: \"%s\" reply: \"%s\"\n", __FUNCTION__, cmd,
          (ret > 0) ? szResponse : "(null)");

  return (ret);
}

int FccdCamera::ShiftDataModulePhase(CameraDriver& driver,
				     int moduleNum) {
  int ret;

  switch (moduleNum) {
  case 1:
    printf(">> Shift clock phase of Data Module 1...\n");
    if ((ret = SendFccdCommand(driver,"10:0a")) < 0) {
      printf(">> Failed to shift clock phase of Data Module 1.\n");
    }
    break;
  case 2:
    printf(">> Shift clock phase of Data Module 2...\n");
    if ((ret = SendFccdCommand(driver,"11:0a")) < 0) {
      printf(">> Failed to shift clock phase of Data Module 2.\n");
    }
    break;
  default:
    printf(">> %s: moduleNum %d is invalid.\n",  __FUNCTION__, moduleNum);
    ret = -1;   // Error
    break;
  }

  return (ret);
}

int FccdCamera::configure(CameraDriver& driver,
			  UserMessage* msg) {
  char sendBuf[SENDBUF_MAXLEN];
  int ret = 0;
  int trace;
  char *initCmd[] = {
    // -- 
    // --	SET State Parameters to fix fCRIC Pipe
    // --
    // -- Change S2(exp) next state from 3 to 5
    "0c:13:05",
    // -- Change S4 next state from 0 to 7
    "0c:23:07",
    // -- Change clks per tick in S3
    "0c:18:04",
    // -- Init S5
    "0c:28:04",
    "0c:29:10",
    // -- change pipeline from 7 to 6
    "0c:2a:06",
    "0c:2b:06",
    "0c:2c:00",
    "0c:2d:00",
    // -- Init S6
    "0c:30:04",
    "0c:31:10",
    // -- change pipeline from 5 to 6
    // -- S5 passes and S6 passes should = 12
    "0c:32:06",
    "0c:33:04",
    "0c:34:00",
    "0c:36:00",
    // -- Init S7
    "0c:38:04",
    "0c:39:10",
    // --0x0c 0x3a 0x07 this was giving an extra pixel
    "0c:3a:07",
    "0c:3b:00",
    "0c:3c:00",
    "0c:3d:00",
    // --
    // -- SET CCD VOLTAGES
    // --
    // -- turn off ccd
    "04:00",
    // empty string marks end of list
    ""
  };

  char *singleCmd[] = {
    // -- Single Mode = Focus bit = 0 and Number of Images = 1
    // ----- Number of Exposures to take after a trigger = 1
    "0e:00:01",
    // ----- Clear focus bit
    "13:00",
    // empty string marks end of list
    ""
  };

  char *focusCmd[] = {
    // ----- Continuous trigger mode (focus):
    // ----- Specify zero images
    "0e:00:00",
    // ----- Set focus bit
    "13:01",
    // -- Make sure the Exposure Delay is set to zero (Should be default condition)
    "0f:00:00:00",
    // empty string marks end of list
    ""
  };

  char *enableCcdCmd[] = {
    // --
    // -- INITIALIZE fCRICS
    // --
    // -- Reset fCRICs and data modules and mask all LVDS
    "10:04:ff:ff",
    "11:04:ff:ff",
    // --10:f1
    // --11:f1
    "10:f0",
    "11:f0",
    "12",
    // -- Write SMDebug
    "10:10:A0:00",
    "10:11:00:87",
    "10:12:00:06",
    "11:10:A0:00",
    "11:11:00:87",
    "11:12:00:06",
    // -- Write Bias currents on
    "10:10:A0:00",
    "10:11:00:81",
    "10:12:00:01",
    "11:10:A0:00",
    "11:11:00:81",
    "11:12:00:01",
    // -- Write Reset ADC
    "10:10:A0:00",
    "10:11:00:85",
    "10:12:00:01",
    "11:10:A0:00",
    "11:11:00:85",
    "11:12:00:01",
    // -- Write 100 MHz readout clock
    "10:10:A0:00",
    "10:11:00:83",
    "10:12:00:02",
    "11:10:A0:00",
    "11:11:00:83",
    "11:12:00:02",
    // -- Write phi1
    "10:10:A0:00",
    "10:11:00:00",
    "10:12:00:42",
    "11:10:A0:00",
    "11:11:00:00",
    "11:12:00:42",
    "10:10:A0:00",
    "10:11:00:01",
    "10:12:00:AE",
    "11:10:A0:00",
    "11:11:00:01",
    "11:12:00:AE",
    "10:10:A0:00",
    "10:11:00:02",
    "10:12:00:FF",
    "11:10:A0:00",
    "11:11:00:02",
    "11:12:00:FF",
    "10:10:A0:00",
    "10:11:00:03",
    "10:12:00:00",
    "11:10:A0:00",
    "11:11:00:03",
    "11:12:00:00",
    // -- Write phi2
    "10:10:A0:00",
    "10:11:00:08",
    "10:12:00:BF",
    "11:10:A0:00",
    "11:11:00:08",
    "11:12:00:BF",
    "10:10:A0:00",
    "10:11:00:09",
    "10:12:00:35",
    "11:10:A0:00",
    "11:11:00:09",
    "11:12:00:35",
    // -- Write VrefSH
    "10:10:A0:00",
    "10:11:00:10",
    "10:12:00:00",
    "11:10:A0:00",
    "11:11:00:10",
    "11:12:00:00",
    "10:10:A0:00",
    "10:11:00:11",
    "10:12:00:FF",
    "11:10:A0:00",
    "11:11:00:11",
    "11:12:00:FF",
    // -- Write Khi
    "10:10:A0:00",
    "10:11:00:18",
    "10:12:00:A7",
    "11:10:A0:00",
    "11:11:00:18",
    "11:12:00:A7",
    "10:10:A0:00",
    "10:11:00:19",
    "10:12:00:BE",
    "11:10:A0:00",
    "11:11:00:19",
    "11:12:00:BE",
    // -- Write CompVeto
    "10:10:A0:00",
    "10:11:00:20",
    "10:12:00:00",
    "11:10:A0:00",
    "11:11:00:20",
    "11:12:00:00",
    "10:10:A0:00",
    "10:11:00:21",
    "10:12:00:FF",
    "11:10:A0:00",
    "11:11:00:21",
    "11:12:00:FF",
    // -- Write CompReset
    "10:10:A0:00",
    "10:11:00:28",
    "10:12:00:02",
    "11:10:A0:00",
    "11:11:00:28",
    "11:12:00:02",
    "10:10:A0:00",
    "10:11:00:29",
    "10:12:00:BF",
    "11:10:A0:00",
    "11:11:00:29",
    "11:12:00:BF",
    // -- Write int reset
    "10:10:A0:00",
    "10:11:00:30",
    "10:12:00:BF",
    "11:10:A0:00",
    "11:11:00:30",
    "11:12:00:BF",
    "10:10:A0:00",
    "10:11:00:31",
    "10:12:00:01",
    "11:10:A0:00",
    "11:11:00:31",
    "11:12:00:01",
    // -- Write start
    "10:10:A0:00",
    "10:11:00:38",
    "10:12:00:0D",
    "11:10:A0:00",
    "11:11:00:38",
    "11:12:00:0D",
    "10:10:A0:00",
    "10:11:00:39",
    "10:12:00:3F",
    "11:10:A0:00",
    "11:11:00:39",
    "11:12:00:3F",
    "10:10:A0:00",
    "10:11:00:3A",
    "10:12:00:71",
    "11:10:A0:00",
    "11:11:00:3A",
    "11:12:00:71",
    "10:10:A0:00",
    "10:11:00:3B",
    "10:12:00:A3",
    "11:10:A0:00",
    "11:11:00:3B",
    "11:12:00:A3",
    // -- Write straight
    "10:10:A0:00",
    "10:11:00:40",
    "10:12:00:C6",
    "11:10:A0:00",
    "11:11:00:40",
    "11:12:00:C6",
    "10:10:A0:00",
    "10:11:00:41",
    "10:12:00:5D",
    "11:10:A0:00",
    "11:11:00:41",
    "11:12:00:5D",
    // -- Write clampr
    "10:10:A0:00",
    "10:11:00:48",
    "10:12:00:C7",
    "11:10:A0:00",
    "11:11:00:48",
    "11:12:00:C7",
    "10:10:A0:00",
    "10:11:00:49",
    "10:12:00:4C",
    "11:10:A0:00",
    "11:11:00:49",
    "11:12:00:4C",
    // -- Write clamp
    "10:10:A0:00",
    "10:11:00:50",
    "10:12:00:B4",
    "11:10:A0:00",
    "11:11:00:50",
    "11:12:00:B4",
    "10:10:A0:00",
    "10:11:00:51",
    "10:12:00:02",
    "11:10:A0:00",
    "11:11:00:51",
    "11:12:00:02",
    // -- Write ac on
    "10:10:A0:00",
    "10:11:00:58",
    "10:12:00:01",
    "11:10:A0:00",
    "11:11:00:58",
    "11:12:00:01",
    "10:10:A0:00",
    "10:11:00:59",
    "10:12:00:4C",
    "11:10:A0:00",
    "11:11:00:59",
    "11:12:00:4C",
    "10:10:A0:00",
    "10:11:00:5A",
    "10:12:00:64",
    "11:10:A0:00",
    "11:11:00:5A",
    "11:12:00:64",
    "10:10:A0:00",
    "10:11:00:5B",
    "10:12:00:B0",
    "11:10:A0:00",
    "11:11:00:5B",
    "11:12:00:B0",
    // -- Write fe reset
    "10:10:A0:00",
    "10:11:00:60",
    "10:12:00:B8",
    "11:10:A0:00",
    "11:11:00:60",
    "11:12:00:B8",
    "10:10:A0:00",
    "10:11:00:61",
    "10:12:00:02",
    "11:10:A0:00",
    "11:11:00:61",
    "11:12:00:02",
    // -- Write timing end
    "10:10:A0:00",
    "10:11:00:06",
    "10:12:00:C8",
    "11:10:A0:00",
    "11:11:00:06",
    "11:12:00:C8",
    // -- Write multislope
    "10:10:A0:00",
    "10:11:00:86",
    "10:12:00:00",
    "11:10:A0:00",
    "11:11:00:86",
    "11:12:00:00",
    // -- Write CAL
    "10:10:A0:00",
    "10:11:00:82",
    "10:12:00:00",
    "11:10:A0:00",
    "11:11:00:82",
    "11:12:00:00",
    // -- mask fCRICs that don't work (none for now)
    "10:04:f0:00",
    "11:04:f0:00",
    "10:f0",
    "11:f0",
    "10:f0",
    "11:f0",
    // -- turn on CCDs here?
    "04:01",
    // -- Descramble right to left
    "10:03:07",
    "11:03:07",
    // empty string marks end of list
    ""
  };

  CurrentCount = RESET_COUNT;

  // enable manual configuration
  switch (_inputConfig->outputMode()) {
    case 1:
      printf(">> Output mode 1: Skip %s\n", __PRETTY_FUNCTION__);
      return (0);

    case 2:
      // shift clock phase of Data Module 1
      ret = ShiftDataModulePhase(driver,1);
      printf(">> Output mode 2: Skip remainder of %s\n", __PRETTY_FUNCTION__);
      return (ret);

    case 3:
      // shift clock phase of Data Module 2
      ret = ShiftDataModulePhase(driver,2);
      printf(">> Output mode 3: Skip remainder of %s\n", __PRETTY_FUNCTION__);
      return (ret);
  }

  //
  // initialize FCCD via CameraLink serial commands
  //

  printf(">> Read FCCD version...\n");
  if (SendFccdCommand(driver,"ff") < 0) {
      printf(">> Failed to read FCCD version.  Is the FCCD powered on?\n");
  }

  printf(">> Set wave form portion of FCCD configuration...\n");

  if ((makeWaveCommand(0, _inputConfig->waveform0(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 0\n");
  }

  if ((makeWaveCommand(1, _inputConfig->waveform1(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 1\n");
  }

  if ((makeWaveCommand(2, _inputConfig->waveform2(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 2\n");
  }

  if ((makeWaveCommand(3, _inputConfig->waveform3(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 3\n");
  }

  if ((makeWaveCommand(4, _inputConfig->waveform4(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 4\n");
  }

  if ((makeWaveCommand(5, _inputConfig->waveform5(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 5\n");
  }

  if ((makeWaveCommand(6, _inputConfig->waveform6(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 6\n");
  }

  if ((makeWaveCommand(7, _inputConfig->waveform7(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 7\n");
  }

  if ((makeWaveCommand(8, _inputConfig->waveform8(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 8\n");
  }

  if ((makeWaveCommand(9, _inputConfig->waveform9(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 9\n");
  }

  if ((makeWaveCommand(10, _inputConfig->waveform10(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 10\n");
  }

  if ((makeWaveCommand(11, _inputConfig->waveform11(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 11\n");
  }

  if ((makeWaveCommand(12, _inputConfig->waveform12(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 12\n");
  }

  if ((makeWaveCommand(13, _inputConfig->waveform13(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 13\n");
  }

  if ((makeWaveCommand(14, _inputConfig->waveform14(), sendBuf) != 0) ||
      (SendFccdCommand(driver,sendBuf) < 0)) {
      printf(">> Failed to configure wave form 14\n");
  }

  printf(">> Set fixed portion of FCCD configuration...\n");
  for (trace = 0; *initCmd[trace]; trace++) {
    if ((ret = SendFccdCommand(driver,initCmd[trace])) < 0) {
      break;
    }
  }

  if (ret >= 0) {
    printf(">> Set variable portion of FCCD configuration...\n");

    if (_inputConfig->ccdEnable()) {
      printf(" >> Set the voltages... \n");

      if ((makeDACWriteCommand(0x00, _inputConfig->dacVoltage1(), FCCD_DAC1_V_START, FCCD_DAC1_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 1\n");
      }

      if ((makeDACWriteCommand(0x02, _inputConfig->dacVoltage2(), FCCD_DAC2_V_START, FCCD_DAC2_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 2\n");
      }

      if ((makeDACWriteCommand(0x04, _inputConfig->dacVoltage3(), FCCD_DAC3_V_START, FCCD_DAC3_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 3\n");
      }

      if ((makeDACWriteCommand(0x06, _inputConfig->dacVoltage4(), FCCD_DAC4_V_START, FCCD_DAC4_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 4\n");
      }

      if ((makeDACWriteCommand(0x08, _inputConfig->dacVoltage5(), FCCD_DAC5_V_START, FCCD_DAC5_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 5\n");
      }

      if ((makeDACWriteCommand(0x0a, _inputConfig->dacVoltage6(), FCCD_DAC6_V_START, FCCD_DAC6_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 6\n");
      }

      if ((makeDACWriteCommand(0x0c, _inputConfig->dacVoltage7(), FCCD_DAC7_V_START, FCCD_DAC7_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 7\n");
      }

      if ((makeDACWriteCommand(0x0e, _inputConfig->dacVoltage8(), FCCD_DAC8_V_START, FCCD_DAC8_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 8\n");
      }

      if ((makeDACWriteCommand(0x10, _inputConfig->dacVoltage9(), FCCD_DAC9_V_START, FCCD_DAC9_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 9\n");
      }

      if ((makeDACWriteCommand(0x12, _inputConfig->dacVoltage10(), FCCD_DAC10_V_START, FCCD_DAC10_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 10\n");
      }

      if ((makeDACWriteCommand(0x14, _inputConfig->dacVoltage11(), FCCD_DAC11_V_START, FCCD_DAC11_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 11\n");
      }

      if ((makeDACWriteCommand(0x16, _inputConfig->dacVoltage12(), FCCD_DAC12_V_START, FCCD_DAC12_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 12\n");
      }

      if ((makeDACWriteCommand(0x18, _inputConfig->dacVoltage13(), FCCD_DAC13_V_START, FCCD_DAC13_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 13\n");
      }

      if ((makeDACWriteCommand(0x1a, _inputConfig->dacVoltage14(), FCCD_DAC14_V_START, FCCD_DAC14_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 14\n");
      }

      if ((makeDACWriteCommand(0x1c, _inputConfig->dacVoltage15(), FCCD_DAC15_V_START, FCCD_DAC15_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 15\n");
      }

      if ((makeDACWriteCommand(0x1e, _inputConfig->dacVoltage16(), FCCD_DAC16_V_START, FCCD_DAC16_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 16\n");
      }

      if ((makeDACWriteCommand(0x20, _inputConfig->dacVoltage17(), FCCD_DAC17_V_START, FCCD_DAC17_V_END, sendBuf) != 0) ||
          (SendFccdCommand(driver,sendBuf) < 0)) {
        printf(">> Failed to configure DAC 17\n");
      }

      printf(" >> Enable CCD... \n");
      for (trace = 0; *enableCcdCmd[trace]; trace++) {
        if ((ret = SendFccdCommand(driver,enableCcdCmd[trace])) < 0) {
          break;
        }
      }
    } else {
      printf(">> Pulse reset_fCRIC_sync...\n");
      // one benefit is reseting the frame counters to 0
      (void) SendFccdCommand(driver,"12");
    }

    if (_inputConfig->focusMode()) {
      printf(">> Focus mode is enabled\n");
      for (trace = 0; *focusCmd[trace]; trace++) {
        if ((ret = SendFccdCommand(driver,focusCmd[trace])) < 0) {
          break;
        }
      }
    } else {
      printf(">> Focus mode is NOT enabled\n");
      for (trace = 0; *singleCmd[trace]; trace++) {
        if ((ret = SendFccdCommand(driver,singleCmd[trace])) < 0) {
          break;
        }
      }
    }

    printf(">> Set internal exposure time...\n");
    sprintf(sendBuf, "08:%02x:%02x:%02x",
            (_inputConfig->exposureTime() & 0xff0000) >> 16,
            (_inputConfig->exposureTime() & 0x00ff00) >> 8,
            (_inputConfig->exposureTime() & 0x0000ff));
    if (SendFccdCommand(driver,sendBuf) < 0) {
      printf(">> Failed to set internal exposure time\n");
    }

    if (_inputConfig->focusMode()) {
      printf(">> Set exposure cycle time to 512 ms...\n");
      if (SendFccdCommand(driver,"09:00:02:00") < 0) {
        printf(">> Failed to set exposure cycle time\n");
      }
    } else {
      printf(">> Set exposure cycle time to 0 ms...\n");
      if (SendFccdCommand(driver,"09:00:00:00") < 0) {
        printf(">> Failed to set exposure cycle time\n");
      }
    }
  }

  if ((!_inputConfig->ccdEnable()) || (_inputConfig->outputMode() == 4)) {
    printf(">> Set camera link output source to test pattern 03 04...\n");
    if (SendFccdCommand(driver,"03:04") < 0) {
      printf(">> Failed to set camera link output source\n");
    }
  } else if (_inputConfig->outputMode() == 0) {
    printf(">> Set camera link output source to CCD...\n");
    if (SendFccdCommand(driver,"03:00") < 0) {
      printf(">> Failed to set camera link output source\n");
    }
  }

  return (ret);
}

bool FccdCamera::validate(Pds::FrameServerMsg& msg)
{
  unsigned short *data = (unsigned short *)msg.data;
  uint32_t Count = (uint32_t)data[0];
  
  if (CurrentCount==RESET_COUNT) {
    CurrentCount = 0;
    LastCount = Count;
  }
  else {
    CurrentCount = Count - LastCount;
  }
  
  if ((CurrentCount&0xffff) != (msg.count&0xffff)) {
    printf("Camera frame number(%ld) != Server number(%d)\n",
           CurrentCount, msg.count);
    return false;
  }
  return true;
}
