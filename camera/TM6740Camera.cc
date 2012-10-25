#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define DBUG

using namespace Pds;

TM6740Camera::TM6740Camera() :
  _inputConfig(0)
{
}

TM6740Camera::~TM6740Camera()
{
}

void TM6740Camera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const TM6740ConfigType*>(p);
  //  ConfigReset();
}

bool TM6740Camera::validate(Pds::FrameServerMsg& msg)
{
  return true;
}

int           TM6740Camera::baudRate() const { return 9600; }
char          TM6740Camera::eotWrite() const { return '\r'; }
char          TM6740Camera::eotRead () const { return '\r'; }
char          TM6740Camera::sof     () const { return ':'; }
char          TM6740Camera::eof     () const { return '\r'; }
unsigned long TM6740Camera::timeout_ms() const { return 100; }

int  TM6740Camera::camera_depth () const { return _inputConfig ? _inputConfig->output_resolution_bits() : 8; }
int  TM6740Camera::camera_taps  () const { return 2; }
const char* TM6740Camera::camera_name() const 
{
  switch(camera_depth()) {
  case 10: return "Pulnix_TM6740CL_10bit";
  case  8:
  default: return "Pulnix_TM6740CL_8bit";
  }
}

int TM6740Camera::camera_height() const
{ return TM6740ConfigType::Row_Pixels >> unsigned(_inputConfig->vertical_binning()); }

int TM6740Camera::camera_width() const
{ return TM6740ConfigType::Column_Pixels >> unsigned(_inputConfig->horizontal_binning()); }

#define SetParameter(title,sfmt,val1) { \
  snprintf(szCommand, SZCOMMAND_MAXLEN, sfmt, val1);	\
  printf("Setting %s [%s]\n",title,szCommand);				\
  ret = driver.SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
}

#define GetParameter(title,gcmd,gfmt,val1) {	\
  snprintf(szCommand, SZCOMMAND_MAXLEN, gcmd);	\
  ret = driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, gfmt, val1); \
  printf("%.*s / %.*s (%d)\n",ret-1,szCommand,ret-1,szResponse,ret);	\
  if (strncmp(szCommand,szResponse,ret-1)) { \
    printf("Error verifying %s [%s/%.*s]\n",gcmd,szCommand,ret-1,szResponse); \
    return -1; \
  } \
}

int TM6740Camera::configure(CameraDriver& driver,
			    UserMessage*  msg) {
  #define SZCOMMAND_MAXLEN 64
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  SetParameter("VREF_A"  ,":VRA=%03X", _inputConfig->vref_a());
  GetParameter("VREF_A"  ,":VRA?", ":oVA%03X", _inputConfig->vref_a());
  SetParameter("GAIN_A"  ,":MGA=%03X", _inputConfig->gain_a());
  GetParameter("GAIN_A"  ,":MGA?", ":oGA%03X", _inputConfig->gain_a());
  SetParameter("VREF_B"  ,":VRB=%03X", _inputConfig->vref_b());
  GetParameter("VREF_B"  ,":VRB?", ":oVB%03X", _inputConfig->vref_b());
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
