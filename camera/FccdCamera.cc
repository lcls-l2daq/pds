//! FccdCamera.cc
//! See FccdCamera.hh for a description of what the class can do
//!
//! Copyright 2010, SLAC
//! Author: caf@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/FccdCamera.hh"
#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <new>

using namespace PdsLeutron;

FccdCamera::FccdCamera(char *id, unsigned grabberId, const char *grabberName) :
  PicPortCL(grabberId, grabberName),
  _inputConfig(0)
{
  if (id == NULL)
    id = "";
  status.CameraId = (char *)malloc(strlen(id)+1);
  strcpy(status.CameraId, id);
  status.CameraName = FCCD_NAME;
  LastCount = 0;
}

FccdCamera::~FccdCamera() {
  free(status.CameraId);
}

void FccdCamera::Config(const FccdConfigType& config)
{
  _inputConfig = &config;
  ConfigReset();
}  

unsigned FccdCamera::output_resolution() const 
{ return Pds::FCCD::FccdConfigV1::Sixteen_bit; }

unsigned    FccdCamera::pixel_rows         () const
{ return Pds::FCCD::FccdConfigV1::Row_Pixels; }

unsigned    FccdCamera::pixel_columns      () const
{ return Pds::FCCD::FccdConfigV1::Column_Pixels; }

const FccdConfigType& FccdCamera::Config() const
{ return *reinterpret_cast<const FccdConfigType*>(_outputBuffer); }

const char* FccdCamera::Name() const
{
  return FCCD_NAME;
}

bool FccdCamera::trigger_CC1() const { return false; }

unsigned FccdCamera::trigger_duration_us() const { return 0; }

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

int FccdCamera::PicPortCameraInit() {
  #define SZRESPONSE_MAXLEN  64
  char szResponse[SZRESPONSE_MAXLEN];
  #define SENDBUF_MAXLEN     10
  char sendBuf[SENDBUF_MAXLEN];
  char pingCmd = 0xff;
  char *initCmd[] = {
    // --
    // -- SET CLOCKS
    // --
    // -- set Hclk1 Idle Pattern
    "0a:00:fe:3f",
    // -- set Hclk2 Idle Pattern
    "0a:01:03:80",
    // -- set Hclk3 Idle Pattern
    "0a:02:f8:ff",
    // -- set sw Idle Pattern
    "0a:03:00:04",
    // -- set rg Idle Pattern
    "0a:04:f8:3f",
    // -- set vclk1 Idle-rd Pattern
    "0a:05:f1:ff",
    // -- set vclk2 Idle-rd Pattern
    "0a:06:1c:00",
    // -- set vclk3 Idle-rd Pattern
    "0a:07:c7:ff",
    // -- set TG Idle-rd Pattern
    "0a:08:83:ff",
    // -- set Hclk1 Pattern
    "0a:09:fe:3f",
    // -- set Hclk2 Pattern
    "0a:0a:03:80",
    // -- set Hclk3 Pattern
    "0a:0b:f8:ff",
    // -- set SW  Pattern
    "0a:0c:00:01",
    // -- set RG  Pattern
    "0a:0d:f8:3f",
    // -- set Convt Pattern
    "0a:0e:00:20",
    // -- 
    // --	SET State Parameters to fix fCRIC Pipe
    // --
    // -- Change S1(25nsec_per_tick) from 50 to 04 or 05 to match state S4
    "0c:00:04",
    // -- Change S1 25nsec_per_tick from ? to 04
    "0c:08:04",
    // -- Change S2(exp) next state from 3 to 5
    "0c:13:05",
    // -- Change clks per tick in S3
    "0c:18:04",
    // -- Change clks per tick in S4 (Thinking about making 4 instead of 5)
    "0c:20:04",
    // -- Change S4 next state from 0 to 7
    "0c:23:07",
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
    "0c:3a:06",
    "0c:3b:00",
    "0c:3c:00",
    "0c:3d:00",
    // --
    // -- SET CCD VOLTAGES
    // --
    // -- turn off ccd
    "04:00",
    // -- begin steps 2 - 5
    // -- 0x08 Internal Exposure time
    // ---- The time is in milliseconds
    "08:00:00:01",
    // -- 0x09 Exposure Cycle Time
    // ---- If you set aa, bb, and cc to zero, the repeat exposures stop.
    "09:00:00:00",
    // -- Single Mode = Focus bit = 0 and Number of Images = 1
    // ----- Number of Exposures to take after a trigger = 1
    "0e:00:01",
    // ----- Clear focus bit
    "13:00",
    // -- Make sure the Exposure Delay is set to zero (Should be default condition)
    "0f:00:00:00",
    // -- Set output to Test Pattern 4
    "03:04",
    // -- END OF CONFIG FILE --
    // empty string marks end of list
    ""
  };
    
  //
  // read the FCCD version via CameraLink serial command
  //
  int ret = SendBinary(&pingCmd, 1, szResponse, SZRESPONSE_MAXLEN);
  printf(">> FCCD version: ");
  if (ret > 0) {
    rstrip(szResponse, ret, eotRead(), 'z');
    printf("%s\n", szResponse);
  } else {
    printf("Error (%d)\n", ret);
  }

  //
  // initialize FCCD via CameraLink serial commands
  //
  for (int trace = 0; *initCmd[trace]; trace++) {
    unsigned int ubuf;
    // format of command string is aa:bb:cc ...
    int sendLen = (strlen(initCmd[trace])+1) / 3;
    for (int ii = 0; (ii < sendLen) && (ii < SENDBUF_MAXLEN); ii++) {
      if (sscanf(initCmd[trace]+(ii*3), "%2x", &ubuf) == 1) {
        sendBuf[ii] = (char)ubuf;
      } else {
        printf(">> %s: Error reading cmd %d element %d\n", __FUNCTION__, trace, ii);
        return (-EINVAL);
      }
    }
    ret = SendBinary(sendBuf, sendLen, szResponse, SZRESPONSE_MAXLEN);
    rstrip(szResponse, ret, eotRead(), 'z');
    printf(">> %s: cmd: \"%s\" reply: \"%s\"\n", __FUNCTION__, initCmd[trace],
            (ret > 0) ? szResponse : "(null)");
  }

  return 0;
}

FrameHandle* 
FccdCamera::PicPortFrameProcess(FrameHandle *pFrame) {
  // do nothing, currently
  return pFrame;
}
