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

using namespace PdsLeutron;

Opal1kCamera::Opal1kCamera(char *id) :
  _inputConfig(0)
{
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
  ConfigReset();
}  

unsigned Opal1kCamera::output_resolution() const 
{ return _inputConfig->output_resolution_bits(); }

const Opal1kConfigType& Opal1kCamera::Config() const
{ return *reinterpret_cast<const Opal1kConfigType*>(_outputBuffer); }

const char* Opal1kCamera::Name() const
{
  if (!_inputConfig) {
    printf("Opal1kCamera::Name() referenced before configuration known\n");
    return 0;
  }
  const char* name(0);
  switch(_inputConfig->output_resolution_bits()) {
  case  8: name = OPAL1000_NAME_8bits; break;
  case 10: name = OPAL1000_NAME_10bits; break;
  case 12: name = OPAL1000_NAME_12bits; break;
  default: name = 0; break;
  }
  return name;
}

bool Opal1kCamera::trigger_CC1() const { return false; }

unsigned Opal1kCamera::trigger_duration_us() const { return 0; }

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

