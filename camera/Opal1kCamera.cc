//! Opal1kCamera.cc
//! See Opal1kCamera.hh for a description of what the class can do
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/Opal1kCamera.hh"
#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <new>

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

using namespace PdsLeutron;

Opal1kCamera::Opal1kCamera(char *id) :
  _inputConfig(0)
{
  PicPortCLConfig.Baudrate = OPAL1000_SERIAL_BAUDRATE;
  PicPortCLConfig.Parity = OPAL1000_SERIAL_PARITY;
  PicPortCLConfig.ByteSize = OPAL1000_SERIAL_DATASIZE;
  PicPortCLConfig.StopSize = OPAL1000_SERIAL_STOPSIZE;
  PicPortCLConfig.eotWrite = OPAL1000_SERIAL_ACK;
  PicPortCLConfig.eotRead = OPAL1000_SERIAL_EOT;
  PicPortCLConfig.sof = OPAL1000_SERIAL_SOF;
  PicPortCLConfig.eof = OPAL1000_SERIAL_EOT;
  PicPortCLConfig.Timeout_ms = OPAL1000_SERIAL_TIMEOUT;
  //  PicPortCLConfig.bUsePicportCounters = false;
  if (id == NULL)
    id = "";
  status.CameraId = (char *)malloc(strlen(id)+1);
  strcpy(status.CameraId, id);
  status.CameraName = OPAL1000_NAME;
  LastCount = 0;
}

Opal1kCamera::~Opal1kCamera() {
  free(status.CameraId);
}

void Opal1kCamera::Config(const Opal1kConfigType& config)
{
  _inputConfig = &config;
  switch(config.output_resolution_bits()) {
  case 8 :
    frameFormat = FrameHandle::FORMAT_GRAYSCALE_8 ; break;
  case 10:
    frameFormat = FrameHandle::FORMAT_GRAYSCALE_10; break;
  case 12:
    frameFormat = FrameHandle::FORMAT_GRAYSCALE_12; break;
  }
  ConfigReset();
}  

const Opal1kConfigType& Opal1kCamera::Config() const
{
  return *reinterpret_cast<const Opal1kConfigType*>(_outputBuffer);
}

int Opal1kCamera::PicPortCameraConfig(LvROI &Roi) {
  U16BIT CameraId;

  // Set config
  SeqDralConfig.NrImages = 16; // 2MB per image !
  SeqDralConfig.UseCameraList = true;
  switch(_inputConfig->output_resolution_bits()) {
    case 8:
      CameraId = DsyGetCameraId(OPAL1000_NAME_8bits);
      Roi.SetColorFormat(ColF_Mono_8);
      break;
    case 10:
      CameraId = DsyGetCameraId(OPAL1000_NAME_10bits);
      Roi.SetColorFormat(ColF_Mono_10);
      break;
    case 12:
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

#define SetCommand(title,cmd) { \
  snprintf(szCommand, SZCOMMAND_MAXLEN, cmd); \
  if ((ret = SendCommand(szCommand, NULL, 0))<0) { \
    printf("Error on command %s (%s)\n",cmd,title); \
    return ret; \
  } \
}

#define GetParameter(cmd,val1) { \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  sscanf(szResponse+2,"%u",&val1); \
}

#define SetParameter(title,cmd,val1) { \
  unsigned val = val1; \
  printf("Setting %s = d%u\n",title,val); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val); \
  ret = SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  unsigned rval; \
  GetParameter(cmd,rval); \
  printf("Read %s = d%u\n", title, rval); \
  if (rval != val) return -EINVAL; \
}

#define GetParameters(cmd,val1,val2) { \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  sscanf(szResponse+2,"%u;%u",&val1,&val2); \
}

#define SetParameters(title,cmd,val1,val2) { \
  unsigned v1 = val1; \
  unsigned v2 = val2; \
  printf("Setting %s = d%u;d%u\n",title,v1,v2); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u;%u",cmd,v1,v2); \
  ret = SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  unsigned rv1,rv2; \
  GetParameters(cmd,rv1,rv2); \
  printf("Read %s = d%u;d%u\n", title, rv1,rv2); \
  if (rv1 != v1 || rv2 != v2) return -EINVAL; \
}

int Opal1kCamera::PicPortCameraInit() {
  #define SZCOMMAND_MAXLEN  20
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  Opal1kConfigType* outputConfig = new (_outputBuffer) Opal1kConfigType(*_inputConfig);
  SetParameter("Black Level" ,"BL",_inputConfig->black_level());
  SetParameter("Digital Gain","GA",_inputConfig->gain_percent());
  SetParameter("Vertical Binning","VBIN",_inputConfig->vertical_binning());
  SetParameter("Output Mirroring","MI",_inputConfig->output_mirroring());
  SetParameter("Vertical Remap"  ,"VR",_inputConfig->vertical_remapping());
  SetParameter("Output Resolution","OR",_inputConfig->output_resolution_bits());
  if (_inputConfig->output_lookup_table_enabled()) {
    SetCommand("Output LUT Begin","OLUTBGN");
    const unsigned short* lut = _inputConfig->output_lookup_table();
    const unsigned short* end = lut + 4096;
    while( lut < end ) {
      snprintf(szCommand, SZCOMMAND_MAXLEN, "OLUT%hu", *lut++);
      if ((ret = SendCommand(szCommand, NULL, 0))<0)
	return ret;
    }
    SetCommand("Output LUT Begin","OLUTEND");
    SetParameter("Output LUT Enabled","OLUTE",1);
  }
  else
    SetParameter("Output LUT Enabled","OLUTE",0);
  if (_inputConfig->defect_pixel_correction_enabled()) {
    //  read defect pixels into output config
    unsigned n,col,row;
    char cmdb[8];
    GetParameter("DP0",n);
    Pds::Camera::FrameCoord* pc = outputConfig->defect_pixel_coordinates();
    for(unsigned k=1; k<=n; k++,pc++) {
      sprintf(cmdb,"DP%d",k);
      GetParameters(cmdb,col,row);
      pc->column = col;
      pc->row = row;
    }
    outputConfig->set_number_of_defect_pixels(n);
    SetParameter("Defect Pixel Correction","DPE",1);
  }
  else
    SetParameter("Defect Pixel Correction","DPE",0);
  SetParameter("Overlay Function","OVL",1);

#if 0  
  // Continuous Mode
  SetParameter ("Operating Mode","MO",0);
  SetParameter ("Frame Period","FP",
	        100000/(unsigned long)config.FramesPerSec);
  SetParameter ("Integration Time","IT",
	        config.ShutterMicroSec/10);
#else
  SetParameter ("Operating Mode","MO",1);
  SetParameters("External Inputs","CCE",4,1);
#endif

  CurrentCount = RESET_COUNT;

  return 0;
}

// We redefine this API to be able to detect dropped/repeated frames
// because the frame grabber is not very good at that. To do that we 
// read the 2 words signature embedded in the frame by the Opal 1000.
FrameHandle* 
Opal1kCamera::PicPortFrameProcess(FrameHandle *pFrame) {
  unsigned long Count;

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

  if (CurrentCount==RESET_COUNT) {
    CurrentCount = 0;
    LastCount = Count;
  }
  else
    CurrentCount = Count - LastCount;

  return pFrame;
}

