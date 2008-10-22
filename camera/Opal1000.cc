//! Opal1000.cc
//! See Opal1000.hh for a description of what the class can do
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "Opal1000.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

// ========================================================
// FindConnIndex
// ========================================================
// Utility function to find connector index (if exists)
// based on the connector name and previously found
// grabber and camera IDs.
// ========================================================
U16BIT FindConnIndex(HGRABBER hGrabber, U16BIT CameraId) {
  U16BIT ConnIndex = HANDLE_INVALID;
  LvGrabberNode *Grabber=DsyGetGrabberPtrFromHandle(hGrabber);
  int NrFree=Grabber->GetNrFreeConnectorEx(CameraId);

  if (NrFree) {
    LvConnectionInfo *ConnectorInfo=new LvConnectionInfo[NrFree];
    // Copy the info to the array
    Grabber->GetFreeConnectorInfoEx(CameraId, ConnectorInfo);
    // Check if it is possible to connect the camera to the
    // grabber via the specified connector.
    for(U16BIT Index=0; Index<NrFree; Index++) {
      if (strncmp(ConnectorInfo[Index].Description, OPAL1000_CONNECTOR, strlen(OPAL1000_CONNECTOR))==0) {
        ConnIndex = Index;
        break;
      }
    }
  }
  return ConnIndex;
}

Pds::Opal1000::Opal1000(char *id) {
  PicPortCLConfig.Baudrate = OPAL1000_SERIAL_BAUDRATE;
  PicPortCLConfig.Parity = OPAL1000_SERIAL_PARITY;
  PicPortCLConfig.ByteSize = OPAL1000_SERIAL_DATASIZE;
  PicPortCLConfig.StopSize = OPAL1000_SERIAL_STOPSIZE;
  PicPortCLConfig.eotWrite = OPAL1000_SERIAL_ACK;
  PicPortCLConfig.eotRead = OPAL1000_SERIAL_EOT;
  PicPortCLConfig.sof = OPAL1000_SERIAL_SOF;
  PicPortCLConfig.eof = OPAL1000_SERIAL_EOT;
  PicPortCLConfig.Timeout_ms = OPAL1000_SERIAL_TIMEOUT;
  PicPortCLConfig.bUsePicportCounters = false;
  if (id == NULL)
    id = "";
  status.CameraId = (char *)malloc(strlen(id)+1);
  strcpy(status.CameraId, id);
  status.CameraName = OPAL1000_NAME;
  LastCount = 0;
}

Pds::Opal1000::~Opal1000() {
  free(status.CameraId);
}

int Pds::Opal1000::PicPortCameraConfig(LvROI &Roi) {
  U16BIT CameraId;

  // Check the camera mode
  if (config.Mode == Camera::MODE_EXTTRIGGER)
    return -ENOTSUP;  // Only support external shutter and continuous
  // Set config
  SeqDralConfig.NrImages = 16; // 2MB per image !
  SeqDralConfig.UseCameraList = true;
  switch(config.Format) {
    case Frame::FORMAT_GRAYSCALE_8:
      CameraId = DsyGetCameraId(OPAL1000_NAME_8bits);
      Roi.SetColorFormat(ColF_Mono_8);
      break;
    case Frame::FORMAT_GRAYSCALE_10:
      CameraId = DsyGetCameraId(OPAL1000_NAME_10bits);
      Roi.SetColorFormat(ColF_Mono_10);
      break;
    case Frame::FORMAT_GRAYSCALE_12:
      CameraId = DsyGetCameraId(OPAL1000_NAME_12bits);
      Roi.SetColorFormat(ColF_Mono_12);
      break;
    default:
      return -ENOTSUP;
  };
  SeqDralConfig.ConnectCamera.MasterCameraType = CameraId;
  SeqDralConfig.ConnectCamera.NrCamera = 1;
  SeqDralConfig.ConnectCamera.Camera[0].hGrabber = SeqDralConfig.hGrabber;
  SeqDralConfig.ConnectCamera.Camera[0].ConnIndex = FindConnIndex(SeqDralConfig.hGrabber, CameraId);
  return 0;
}

int Pds::Opal1000::PicPortCameraInit() {
  #define SZCOMMAND_MAXLEN  20
  char szCommand[SZCOMMAND_MAXLEN];
  int ret;

  // Set factory defaults (by loading config 0)
  snprintf(szCommand, SZCOMMAND_MAXLEN, "LC0");
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the black level
  snprintf(szCommand, SZCOMMAND_MAXLEN, "BL%lu", 
                  (((unsigned long)config.BlackLevelPercent*4096)/100)-1);
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the frame period (in 100th of ms)
  snprintf(szCommand, SZCOMMAND_MAXLEN, "FP%lu",
                                100000/(unsigned long)config.FramesPerSec);
  ret = SendCommand(szCommand, NULL, 0);
printf("Sending command %s => %d.\n",szCommand, ret);
  if(ret < 0)
    return ret;
  // Set the digital gain of the camera
  snprintf(szCommand, SZCOMMAND_MAXLEN, "GA%lu",
                        ((unsigned long)config.GainPercent*3100)/100+100);
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the integration time
  snprintf(szCommand, SZCOMMAND_MAXLEN, "IT%lu",config.ShutterMicroSec/10);
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the operating mode
  int mode;
  switch(config.Mode) {
    case Camera::MODE_CONTINUOUS:
      mode = 0;
      break;
    case Camera::MODE_EXTTRIGGER_SHUTTER:
      mode = 1;
      break;
    default:
      return -ENOSYS;  // should not happen
  };
  snprintf(szCommand, SZCOMMAND_MAXLEN, "MO%d", mode);
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the output resolution
  int nbits;
  switch(config.Format) {
    case Frame::FORMAT_GRAYSCALE_8:
      nbits = 8;
      break;
    case Frame::FORMAT_GRAYSCALE_10:
      nbits = 10;
      break;
    case Frame::FORMAT_GRAYSCALE_12:
      nbits = 12;
      break;
    default:
      return -ENOSYS;  // should not happen
  };
  snprintf(szCommand, SZCOMMAND_MAXLEN, "OR%d", nbits);
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Set the vertical remap
  snprintf(szCommand, SZCOMMAND_MAXLEN, "VR1"); // Required by frame-grabber
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  // Enable overlay information
  snprintf(szCommand, SZCOMMAND_MAXLEN, "OVL1");
  ret = SendCommand(szCommand, NULL, 0);
  if(ret < 0)
    return ret;
  return 0;
}

// We redefine this API to be able to detect dropped/repeated frames
// because the frame grabber is not very good at that. To do that we 
// read the 2 words signature embedded in the frame by the Opal 1000.
Pds::Frame *Pds::Opal1000::PicPortFrameProcess(Pds::Frame *pFrame) {
  unsigned long Count;
  unsigned long Delta;

  switch (pFrame->elsize) {
    case 1:
    { unsigned char *data = (unsigned char *)pFrame->data;
      Count = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
      break;
    }
    case 2:
    { unsigned short *data = (unsigned short *)pFrame->data;
      Count = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
      break;
    }
    default:
      /* Not supported */
      return pFrame;
  }

  if (LastCount == 0) {
    // First time we cannot measure the dropped frames
    status.CapturedFrames += 1;
    LastCount = Count;
    return pFrame;
  } else if (Count < LastCount) {
    // Wrapped around
    Delta = ((~0) - LastCount) + Count + 1;
  } else {
    Delta = Count - LastCount;
  }
  if (Delta == 0) {
    /* We read teh same frame twice, forget it */
    delete pFrame;
    return (Pds::Frame *)NULL;
  } else if (Delta > 1) {
    status.DroppedFrames += Delta - 1;
  }
  status.CapturedFrames += Delta;
  LastCount = Count;
  return pFrame;
}

