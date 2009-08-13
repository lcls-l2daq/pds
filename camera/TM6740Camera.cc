//! TM6740Camera.cc
//! See TM6740Camera.hh for a description of what the class can do
//!
//! Copyright 2009, SLAC
//! Author: weaver@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/TM6740Camera.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <new>

using namespace PdsLeutron;

TM6740Camera::TM6740Camera(char *id) :
  _inputConfig(0)
{
  if (id == NULL)
    id = "";
  status.CameraId = (char *)malloc(strlen(id)+1);
  strcpy(status.CameraId, id);
  status.CameraName = TM6740_NAME;
}

TM6740Camera::~TM6740Camera() {
  free(status.CameraId);
}

void TM6740Camera::Config(const TM6740ConfigType& config)
{
  _inputConfig = &config;
  ConfigReset();
}

unsigned TM6740Camera::output_resolution() const 
{ return _inputConfig->output_resolution_bits(); }

unsigned    TM6740Camera::pixel_rows         () const
{ return Pds::Pulnix::TM6740ConfigV1::Row_Pixels >> unsigned(_inputConfig->vertical_binning()); }

unsigned    TM6740Camera::pixel_columns      () const
{ return Pds::Pulnix::TM6740ConfigV1::Column_Pixels >> unsigned(_inputConfig->horizontal_binning()); }

const TM6740ConfigType& TM6740Camera::Config() const
{ return *reinterpret_cast<const TM6740ConfigType*>(_outputBuffer); }

const char* TM6740Camera::Name() const
{
  if (!_inputConfig) {
    printf("TM6740Camera::Name() referenced before configuration known\n");
    return 0;
  }
  return (_inputConfig->output_resolution_bits()==10) ? TM6740_NAME_10bits : TM6740_NAME_8bits;
}

//bool TM6740Camera::trigger_CC1() const { return true; }
bool TM6740Camera::trigger_CC1() const { return false; }

unsigned TM6740Camera::trigger_duration_us() const { return _inputConfig->shutter_width(); }

#define SetParameter(title,sfmt,val1) { \
  snprintf(szCommand, SZCOMMAND_MAXLEN, sfmt, val1);	\
  printf("Setting %s [%s]\n",title,szCommand);				\
  ret = SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
}

#define GetParameter(title,gcmd,gfmt,val1) {	\
  snprintf(szCommand, SZCOMMAND_MAXLEN, gcmd);	\
  ret = SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, gfmt, val1); \
  printf("%.*s / %.*s (%d)\n",ret-1,szCommand,ret-1,szResponse,ret);	\
  if (strncmp(szCommand,szResponse,ret-1)) { \
    printf("Error verifying %s [%s/%.*s]\n",gcmd,szCommand,ret-1,szResponse); \
    return -1; \
  } \
}

int TM6740Camera::PicPortCameraInit() {
  #define SZCOMMAND_MAXLEN 64
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  *new (_outputBuffer) TM6740ConfigType(*_inputConfig);
  SetParameter("VREF_A"  ,":VRA=%03X", _inputConfig->vref());
  GetParameter("VREF_A"  ,":VRA?", ":oVA%03X", _inputConfig->vref());
  SetParameter("GAIN_A"  ,":MGA=%03X", _inputConfig->gain_a());
  GetParameter("GAIN_A"  ,":MGA?", ":oGA%03X", _inputConfig->gain_a());
  SetParameter("VREF_B"  ,":VRB=%03X", _inputConfig->vref());
  GetParameter("VREF_B"  ,":VRB?", ":oVB%03X", _inputConfig->vref());
  SetParameter("GAIN_B"  ,":MGB=%03X", _inputConfig->gain_b());
  GetParameter("GAIN_B"  ,":MGB?", ":oGB%03X", _inputConfig->gain_b());
  SetParameter("GAIN_BAL",":%s"      , _inputConfig->gain_balance() ? "EABL" : "DABL");
  GetParameter("GAIN_BAL",":ABL?", ":oAB%d", _inputConfig->gain_balance() ? 1:0);
  // Hard-code video order 'C'
  SetParameter("VID_ORD" ,":VDO%c", 'C');
  GetParameter("VID_ORD" ,":VDO?", ":oVD%c", 'C');
  SetParameter("Depth"   ,":DDP=%d", _inputConfig->output_resolution());
  // Hard-code shutter mode 'ASH=9' (pulse-width control)
  int shmode = 9;
  SetParameter("ShMode",":ASH=%d",shmode); 
  GetParameter("ShMode",":SHR?",":oAS%d", shmode);
  
  SetParameter("LUT"     ,":%s", _inputConfig->lookuptable_mode()==TM6740ConfigType::Gamma ? "GM45" : "LINR");
  GetParameter("LUT"     ,":LUT?", ":oTBA-%s", _inputConfig->lookuptable_mode()==TM6740ConfigType::Gamma ? "GM" : "LN");
  SetParameter("ScanMode",":SMD%c",'A');
  GetParameter("ScanMode",":SMD?",":oMD%c",'A');

  return 0;
}


