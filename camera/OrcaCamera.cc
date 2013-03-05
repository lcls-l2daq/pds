#include "pds/camera/OrcaCamera.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/camera/FrameCoord.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define RESET_COUNT 0x80000000
#define SZCOMMAND_MAXLEN  64

using namespace Pds;

OrcaCamera::OrcaCamera(const DetInfo& src) :
  _src        (src),
  _inputConfig(0)
{
}

OrcaCamera::~OrcaCamera()
{
}

void OrcaCamera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const OrcaConfigType*>(p);
  switch(_inputConfig->mode()) {
  case OrcaConfigType::Subarray: 
  case OrcaConfigType::x1      : _offset =  100; break;
  case OrcaConfigType::x2      : _offset =  400; break;
  case OrcaConfigType::x4      : _offset = 1600; break;
  default: break;
  }
}

#define SetCommand(title,cmd) {				\
    snprintf(szCommand, SZCOMMAND_MAXLEN, cmd);		\
    if ((ret = driver.SendCommand(szCommand, NULL, 0))<0) {	\
      printf("Error on command %s (%s)\n",cmd,title);	\
      return ret;					\
    }							\
  }

#define GetParameter(title,cmd) {				       \
    snprintf(szCommand, SZCOMMAND_MAXLEN, "?%s", cmd);                 \
    ret = driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
    printf("%s: %s\n", title, &szResponse[strlen(cmd)+1]);	       \
  }

#define SetParameter(title,cmd,val1) {					\
    GetParameter(title,cmd);						\
    unsigned nretry = 1;						\
    do {								\
      snprintf(szCommand, SZCOMMAND_MAXLEN, "%s %s", cmd, val1);	\
      ret = driver.SendCommand(szCommand, NULL, 0);			\
      if (ret<0) return ret;						\
      GetParameter(title,cmd);						\
      szResponse[strlen(szResponse)-1] = 0;				\
    } while ( nretry-- && (strcmp(val1, &szResponse[strlen(cmd)+1])));	\
    if (strcmp(val1, &szResponse[strlen(cmd)+1])) {			\
      printf("[%s] != [%s]\n", val1,szResponse);			\
      return -EINVAL;							\
    }									\
  }

int OrcaCamera::configure(CameraDriver& driver,
			  UserMessage*  msg)
{
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  memset(szCommand ,0,SZCOMMAND_MAXLEN);
  memset(szResponse,0,SZCOMMAND_MAXLEN);
  int ret;

  GetParameter("Camera Name"      ,"CAI T");
  GetParameter("Serial Number"    ,"CAI N" );
  GetParameter( "Total on time"   ,"RAT" );
  GetParameter( "Firmware version","VER" );

  SetParameter("Output Mode (No Bundling)","OMD","A");
  SetParameter("Repeat Imaging"           ,"ACN","0");
  SetParameter("Pulse Skip Number"        ,"ATN","1");

  //  SetParameter("Cooling Mode","CSW","F");

  SetParameter("External Trigger Input Pulse Polarity","ATP","P");
  SetParameter("External Trigger Input Pulse Delay"   ,"ATD","0.0");
  SetParameter("External Trigger Input Pulse Source"  ,"ESC","B");
  SetParameter("Exposure Start","AMD","E");
  SetParameter("Exposure Mode","EMD","L");

  switch(_inputConfig->mode()) {
  case OrcaConfigType::x1: 
    SetParameter("Scan Mode","SMD","N");
    SetParameter("Imaging","ACT","I");
    SetParameter("Binning","SPX","1");
    break;
  case OrcaConfigType::x2:
    SetParameter("Scan Mode","SMD","S");
    SetParameter("Imaging","ACT","I");
    SetParameter("Binning","SPX","2");
    break;
  case OrcaConfigType::x4:
    SetParameter("Scan Mode","SMD","S");
    SetParameter("Imaging","ACT","I");
    SetParameter("Binning","SPX","4");
    break;
  case OrcaConfigType::Subarray:
    SetParameter("Scan Mode","SMD","W");
    { char p[32];
      // SDV bottom_off,top_off,rows
      //   bottom_off: offset from bottom of frame
      //   top_off   : offset from center of frame
      //   rows      : half the number of rows
      // Set as one central region
      unsigned nr = ((_inputConfig->rows()+7)>>3)<<2;
      sprintf(p,"%d,0,%d", OrcaConfigType::Row_Pixels/2-nr, nr);
      SetParameter("Vertical Double Scan","SDV",p);
    }
    SetParameter("Imaging","ACT","I");
    SetParameter("Binning","SPX","1");
    break;
  default:
    msg->append("Unknown ORCA readout mode");
    return -EINVAL;
  }

  if (_inputConfig->defect_pixel_correction_enabled()) {
    SetParameter("DPixel Corr","PEC","O");
  }
  else {
    SetParameter("DPixel Corr","PEC","F");
  }

  GetParameter("Unk","CEG");
  GetParameter("Unk","CEO");
  GetParameter("Unk","SHT");

  CurrentCount = RESET_COUNT;

  return 0;
}

int OrcaCamera::setTestPattern( CameraDriver& driver, bool on )
{
#if 0
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];

  int ret;
  SetParameter( "Test Pattern", "TP", on ? 1 : 0 );
#endif  
  return 0;
}

int OrcaCamera::setContinuousMode( CameraDriver& driver, double fps )
{
#if 0
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  unsigned _mode=0, _fps=unsigned(100000/fps), _it=_fps-10;
  SetParameter ("Operating Mode","MO",_mode);
  SetParameter ("Frame Period","FP",_fps);
  // SetParameter ("Frame Period","FP",813);
  SetParameter ("Integration Time","IT", _it);
  // SetParameter ("Integration Time","IT", 810);
#endif
  return 0;
}

bool OrcaCamera::validate(Pds::FrameServerMsg& msg)
{
  msg.width -= msg.width%camera_taps();
  msg.offset = _offset;
  msg.intlv  = FrameServerMsg::MidTopLine;
  return true;
}

int           OrcaCamera::baudRate() const { return 38400; }
char          OrcaCamera::eotWrite() const { return '\r'; }
char          OrcaCamera::eotRead () const { return '\r'; }
char          OrcaCamera::sof     () const { return '\0'; }
char          OrcaCamera::eof     () const { return '\r'; }
unsigned long OrcaCamera::timeout_ms() const { return 400; }
bool          OrcaCamera::uses_sof() const { return false; }

int  OrcaCamera::camera_width () const 
{ 
  int np = OrcaConfigType::Column_Pixels;
  if (_inputConfig) {
    switch(_inputConfig->mode()) {
    case OrcaConfigType::Subarray: 
    case OrcaConfigType::x1      : break;
    case OrcaConfigType::x2      : np>>=1; break;
    case OrcaConfigType::x4      : np>>=2; break;
    default: break;
    }
  }
  /*
  **  Problems when the frame width is not a multiple of camera taps
  **    Either crop pixels or expand and accept fake pixels
  **   (this problem is likely true for any camera)
  */
  unsigned t = camera_taps();

  return np;
#if 1
  np  = t*( np/t);        // crop
#else
  np  = t*((np+t-1)/t);   // expand
#endif
  return np;
}

int  OrcaCamera::camera_height () const 
{ 
  if (_inputConfig) {
    switch(_inputConfig->mode()) {
    case OrcaConfigType::x1      : return OrcaConfigType::Row_Pixels/1;
    case OrcaConfigType::x2      : return OrcaConfigType::Row_Pixels/2;
    case OrcaConfigType::x4      : return OrcaConfigType::Row_Pixels/4;
    case OrcaConfigType::Subarray: return _inputConfig->rows();
    default:
      break;
    }
  }
  return OrcaConfigType::Row_Pixels; 
}

int  OrcaCamera::camera_depth () const { return 16; }
int  OrcaCamera::camera_taps  () const { return 5; }

int OrcaCamera::camera_cfg2() const { return 0x40; }  // Fast rearm

const char* OrcaCamera::camera_name() const 
{
  return 0;
}
