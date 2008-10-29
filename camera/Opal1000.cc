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

#define RESET_COUNT 0x80000000

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

#define SetParameter(title,cmd,val1) { \
  unsigned long val = val1; \
  printf("Setting %s = d%lu\n",title,val); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%lu", cmd, val); \
  ret = SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  unsigned long rval; \
  sscanf(szResponse+2,"%lu",&rval); \
  printf("Read %s = d%lu\n", title, rval); \
  if (rval != val) return -EINVAL; \
}

#define SetParameters(title,cmd,val1,val2) { \
  unsigned long v1 = val1; \
  unsigned long v2 = val2; \
  printf("Setting %s = d%lu;d%lu\n",title,v1,v2); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%lu;%lu",cmd,v1,v2); \
  ret = SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  unsigned long rv1,rv2; \
  sscanf(szResponse+2,"%lu;%lu",&rv1,&rv2); \
  printf("Read %s = d%lu;d%lu\n", title, rv1,rv2); \
  if (rv1 != v1 || rv2 != v2) return -EINVAL; \
}

int Pds::Opal1000::PicPortCameraInit() {
  #define SZCOMMAND_MAXLEN  20
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  SetParameter("Black Level","BL",
	       (((unsigned long)config.BlackLevelPercent*4096)/100)-1);
  SetParameter("Digital Gain","GA",
	       ((unsigned long)config.GainPercent*3100)/100+100);

  if (config.Mode == MODE_CONTINUOUS) {
    SetParameter("Operating Mode","MO",0);
    SetParameter("Frame Period","FP",
		 100000/(unsigned long)config.FramesPerSec);
    SetParameter("Integration Time","IT",
		 config.ShutterMicroSec/10);
  }
  else if (config.Mode == MODE_EXTTRIGGER_SHUTTER) {
    SetParameter("Operating Mode","MO",1);
    SetParameters("External Inputs","CCE",4,1);
  }
  else
    return -ENOSYS;

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
  SetParameter("Output Resolution","OR",nbits);
  SetParameter("Overlay Function","OVL",1);
  SetParameter("Vertical Remap","VR",1);

  CurrentCount = RESET_COUNT;

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

#if 0
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
#else
  if (CurrentCount==RESET_COUNT) {
    CurrentCount = 0;
    LastCount = Count;
  }
  else
    CurrentCount = Count - LastCount;
#endif
  return pFrame;
}

