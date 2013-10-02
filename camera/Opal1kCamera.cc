#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define DBUG

#define RESET_COUNT 0x80000000
#define SZCOMMAND_MAXLEN  64

using namespace Pds;

Opal1kCamera::Opal1kCamera(const DetInfo& src) :
  _src        (src),
  _inputConfig(0)
{
}

Opal1kCamera::~Opal1kCamera()
{
}

void Opal1kCamera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const Opal1kConfigType*>(p);
}

#define SetCommand(title,cmd) {				\
    snprintf(szCommand, SZCOMMAND_MAXLEN, cmd);		\
    if ((ret = driver.SendCommand(szCommand, NULL, 0))<0) {	\
      printf("Error on command %s (%s)\n",cmd,title);	\
      return ret;					\
    }							\
  }

#define GetParameter(cmd,val1) {				\
    snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd);		\
    ret = driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
    sscanf(szResponse+2,"%u",&val1);				\
  }

#define SetParameter(title,cmd,val1) {				\
    unsigned nretry = 1;					\
    unsigned rval;						\
    unsigned val = val1;					\
    do {							\
      GetParameter(cmd,rval);					\
      printf("Read %s = d%u\n", title, rval);			\
      printf("Setting %s = d%u\n",title,val);			\
      snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val);	\
      ret = driver.SendCommand(szCommand, NULL, 0);			\
      if (ret<0) return ret;					\
      GetParameter(cmd,rval);					\
      printf("Read %s = d%u\n", title, rval);			\
    } while ( nretry-- && (rval != val) );			\
    if (rval != val) return -EINVAL;				\
  }

#define GetParameters(cmd,val1,val2) {				\
    snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd);		\
    ret = driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
    sscanf(szResponse+2,"%u;%u",&val1,&val2);			\
  }

#define SetParameters(title,cmd,val1,val2) {				\
    unsigned nretry = 1;						\
    unsigned v1 = val1;							\
    unsigned v2 = val2;							\
    unsigned rv1,rv2;							\
    do {								\
      printf("Setting %s = d%u;d%u\n",title,v1,v2);			\
      snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u;%u",cmd,v1,v2);	\
      ret = driver.SendCommand(szCommand, NULL, 0);				\
      if (ret<0) return ret;						\
      GetParameters(cmd,rv1,rv2);					\
      printf("Read %s = d%u;d%u\n", title, rv1,rv2);			\
    } while( nretry-- && (rv1!=v1 || rv2!=v2) );			\
    if (rv1 != v1 || rv2 != v2) return -EINVAL;				\
  }

int Opal1kCamera::configure(CameraDriver& driver,
			    UserMessage*  msg)
{
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  memset(szCommand ,0,SZCOMMAND_MAXLEN);
  memset(szResponse,0,SZCOMMAND_MAXLEN);
  int val1=-1, val2=-1;
  int ret;
  char *versionString;
  int versionMajor, versionMinor;
  int versionFPGA = 0;
  bool triggerInverted = false;

  GetParameters("BIT", val1, val2);
  printf( ">> Built-In Self Test (BIT): '%d;%d' %s \n", val1, val2,
          ( val1 == 0 && val2 == 0 ) ? "[OK]" : "[FAIL]" );
  
  GetParameters("TM", val1, val2 );
  printf( ">> Camera temperature (TM): '%d' C, '%d' F\n", val1, val2 );

  GetParameters( "ET", val1, val2 );
  printf( ">> Total on time: '%d' * 65536 + '%d' = %d hours\n", val1, val2,
          val1 * 65536 + val2 );

  GetParameter( "UFDT", val1 );
  printf( ">> Microcontroller firmware release: '%s'\n", szResponse );

  GetParameter( "BS", val1 );
  printf( ">> Build versions: '%s'\n", szResponse );

  // FPGA firmware versions 1.20 and newer have the external
  // trigger polarity inverted.  If FPGA firmware is 1.20 or later,
  // we compensate by inverting the polarity requested by software.

  // Command syntax: BS?
  // Reply message: "x.xx;y.yy;z.zz
  //   Where x.xx stands for camera issue, y.yy indicates the microcontroller firmware version
  //   and z.zz indicates the FPGA firmware version.
  if (((versionString = strchr(szResponse, ';')) != NULL) &&
      ((versionString = strchr(versionString + 1, ';')) != NULL)) {
    // advance to character after the 2nd ';'
    ++versionString;
  }
  if (versionString) {
    versionMajor = atoi(versionString);
    versionMinor = atoi(versionString + 2);
    // sanity check
    if ((versionMajor <= 9) && (versionMinor <= 99)) {
      versionFPGA = (100 * versionMajor) + versionMinor;
    }
  }
  // sanity check
  if ((versionFPGA < 1) || (versionFPGA > 999))
    {
    printf( ">> ERROR: Failed to read FPGA firmware version\n");
    return (-1);
    }

  printf( ">> FPGA firmware version %d.%02d: ", versionMajor, versionMinor);
  if (versionFPGA >= 120) {
    printf( "adjusting for inverted trigger polarity.\n");
    triggerInverted = true;
  } else {
    printf( "trigger polarity is normal.\n");
  }

  GetParameter( "ID", val1 );
  printf( ">> Camera ID: '%s'\n", szResponse );
  
  GetParameter( "MID", val1 );
  printf( ">> Camera Model ID: '%s'\n", szResponse );
  
  GetParameter( "SN", val1 );
  printf( ">> Camera Serial #: '%s'\n", szResponse );

  unsigned bl = _inputConfig->black_level()<<(12-_inputConfig->output_resolution_bits());
  SetParameter("Black Level" ,"BL",bl);
  SetParameter("Digital Gain","GA",_inputConfig->gain_percent());
  SetParameter("Vertical Binning","VBIN",_inputConfig->vertical_binning());
  SetParameter("Output Mirroring","MI",_inputConfig->output_mirroring());
  SetParameter("Vertical Remap"  ,"VR",_inputConfig->vertical_remapping());
  SetParameter("Output Resolution","OR",_inputConfig->output_resolution_bits());

  setTestPattern( driver, false );

  if (_inputConfig->output_lookup_table_enabled()) {
    SetCommand("Output LUT Begin","OLUTBGN");
    ndarray<const uint16_t,1> lut = _inputConfig->output_lookup_table();
    for(unsigned k=0; k<lut.shape()[0]; k++) {
      snprintf(szCommand, SZCOMMAND_MAXLEN, "OLUT%hu", lut[k]);
      if ((ret = driver.SendCommand(szCommand, NULL, 0))<0)
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

    Pds::Camera::FrameCoord* dps = 0;
    if (n) {
      dps = new Pds::Camera::FrameCoord[n];

      for(unsigned k=0; k<n; k++) {
        sprintf(cmdb,"DP%d",k+1);
        GetParameters(cmdb,col,row);
        dps[k] = Pds::Camera::FrameCoord(col,row);
      }

      SetParameter("Defect Pixel Correction","DPE",1);

      Opal1kConfigType* m = const_cast<Opal1kConfigType*>(_inputConfig);
      *new(m) Opal1kConfigType(_inputConfig->output_offset(),
                               _inputConfig->gain_percent(),
                               _inputConfig->output_resolution(),
                               _inputConfig->vertical_binning(),
                               _inputConfig->output_mirroring(),
                               _inputConfig->vertical_remapping(),
                               _inputConfig->output_lookup_table_enabled(),
                               _inputConfig->defect_pixel_correction_enabled(),
                               n,
                               _inputConfig->output_lookup_table().data(),
                               dps);
      delete[] dps;
    }
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
  SetParameters("External Inputs","CCE",4, triggerInverted ? 0 : 1);
#endif

  CurrentCount = RESET_COUNT;

  return 0;
}

int Opal1kCamera::setTestPattern( CameraDriver& driver, bool on )
{
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];

  int ret;
  SetParameter( "Test Pattern", "TP", on ? 1 : 0 );
  
  return 0;
}

int Opal1kCamera::setContinuousMode( CameraDriver& driver, double fps )
{
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];
  int ret;

  unsigned _mode=0, _fps=unsigned(100000/fps), _it=_fps-10;
  SetParameter ("Operating Mode","MO",_mode);
  SetParameter ("Frame Period","FP",_fps);
  // SetParameter ("Frame Period","FP",813);
  SetParameter ("Integration Time","IT", _it);
  // SetParameter ("Integration Time","IT", 810);
  return 0;
}

bool Opal1kCamera::validate(Pds::FrameServerMsg& msg)
{
  uint32_t Count;

  if (camera_depth() <= 8) {
    uint8_t* data = (uint8_t*)msg.data;
    Count = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
  }
  else {
    uint16_t* data = (uint16_t*)msg.data;
    Count = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
  }

  if (CurrentCount==RESET_COUNT) {
    CurrentCount = 0;
    LastCount = Count;
  }
  else
    CurrentCount = Count - LastCount;

#ifdef DBUG
  printf("Camera frame number(%x). Server number(%x), Count(%x), Last count(%x)\n",
	 CurrentCount, msg.count, Count, LastCount);
#else
  if (CurrentCount != msg.count) {
    printf("Camera frame number(%d) != Server number(%d)\n",
           CurrentCount, msg.count);
    return false;
  }
#endif

  msg.offset = _inputConfig->output_offset();
  return true;
}

int           Opal1kCamera::baudRate() const { return 57600; }
char          Opal1kCamera::eotWrite() const { return 0x06; }
char          Opal1kCamera::eotRead () const { return '\r'; }
char          Opal1kCamera::sof     () const { return '@'; }
char          Opal1kCamera::eof     () const { return '\r'; }
unsigned long Opal1kCamera::timeout_ms() const { return 40; }

int  Opal1kCamera::camera_width () const { return Pds::Opal1k::max_column_pixels(_src); }
int  Opal1kCamera::camera_height() const { return Pds::Opal1k::max_row_pixels   (_src); }
int  Opal1kCamera::camera_depth () const { return _inputConfig ? _inputConfig->output_resolution_bits() : 8; }
int  Opal1kCamera::camera_taps  () const { return 2; }

const char* Opal1kCamera::camera_name() const 
{
  switch(_src.device()) {
  case DetInfo::Opal1000:
    switch(camera_depth()) {
    case 12: return "Adimec_Opal-1000m/Q";
    case 10: return "Adimec_Opal-1000m/Q_F10bits";
    case  8: 
    default: return "Adimec_Opal-1000m/Q_F8bit";
    } break;
  case DetInfo::Opal1600:
    switch(camera_depth()) {
    case 12: return "Adimec_Opal-1600m/Q";
    case 10: return "Adimec_Opal-1600m/Q_F10bits";
    case  8: 
    default: return "Adimec_Opal-1600m/Q_F8bit";
    } break;
  case DetInfo::Opal2000:
    switch(camera_depth()) {
    case 12: return "Adimec_Opal-2000m/Q";
    case 10: return "Adimec_Opal-2000m/Q_F10bits";
    case  8: 
    default: return "Adimec_Opal-2000m/Q_F8bit";
    } break;
  case DetInfo::Opal4000:
    switch(camera_depth()) {
    case 12: return "Adimec_Opal-4000m/Q";
    case 10: return "Adimec_Opal-4000m/Q_F10bits";
    case  8: 
    default: return "Adimec_Opal-4000m/Q_F8bit";
    } break;
  case DetInfo::Opal8000:
    switch(camera_depth()) {
    case 12: return "Adimec_Opal-8000m/Q";
    case 10: return "Adimec_Opal-8000m/Q_F10bits";
    case  8: 
    default: return "Adimec_Opal-8000m/Q_F8bit";
    } break;
  default:
    printf("Opal1kCamera::camera_name illegal type %s\n",
           DetInfo::name(_src.device()));
    exit(-1);
    break;
  }
  return 0;
}
