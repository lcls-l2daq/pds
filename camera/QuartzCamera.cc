#include "pds/camera/QuartzCamera.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pdsdata/camera/FrameCoord.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define DBUG

#define RESET_COUNT 0x80000000
#define SZCOMMAND_MAXLEN  64

// Base mode
//#define MAX_TAPS 2
// Medium mode
//#define MAX_TAPS 4
// Full mode
#define MAX_TAPS 8

using namespace Pds;

QuartzCamera::QuartzCamera(const DetInfo& src) :
  _src        (src),
  _inputConfig(0)
{
}

QuartzCamera::~QuartzCamera()
{
}

void QuartzCamera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const QuartzConfigType*>(p);
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

int QuartzCamera::configure(CameraDriver& driver,
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

  QuartzConfigType* outputConfig = const_cast<QuartzConfigType*>(_inputConfig);

  GetParameter("BIT", val1);
  printf( ">> Built-In Self Test (BIT): '%d' %s \n", val1,
          ( val1 == 0) ? "[OK]" : "[FAIL]" );
  
  GetParameter("TM", val1 );
  printf( ">> Camera temperature (TM): '%d' C\n", val1 );

#if 0
  GetParameters( "ET", val1, val2 );
  printf( ">> Total on time: '%d' * 65536 + '%d' = %d hours\n", val1, val2,
          val1 * 65536 + val2 );

  GetParameter( "UFDT", val1 );
  printf( ">> Microcontroller firmware release: '%s'\n", szResponse );
#endif

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
#if 0
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
#else
  printf("versionString: %s\n",versionString);
#endif

  GetParameter( "ID", val1 );
  printf( ">> Camera ID: '%s'\n", szResponse );
  
#if 0
  GetParameter( "MID", val1 );
  printf( ">> Camera Model ID: '%s'\n", szResponse );
#endif
  
  GetParameter( "SN", val1 );
  printf( ">> Camera Serial #: '%s'\n", szResponse );

  unsigned bl = _inputConfig->black_level()<<(10-_inputConfig->output_resolution_bits());
  //  unsigned bl = _inputConfig->black_level();
  SetParameter("Black Level" ,"BL",bl);
  SetParameter("Digital Gain","GA",_inputConfig->gain_percent());
  { unsigned vb;
    switch(_inputConfig->vertical_binning()) {
    case Pds::Quartz::ConfigV1::x1: vb = 1; break;
    case Pds::Quartz::ConfigV1::x2: vb = 2; break;
    case Pds::Quartz::ConfigV1::x4: vb = 4; break;
    default: vb = 0; break;
    }
    SetParameter("Vertical Binning","VBIN",vb);
  }
  { unsigned hb;
    switch(_inputConfig->horizontal_binning()) {
    case Pds::Quartz::ConfigV1::x1: hb = 1; break;
    case Pds::Quartz::ConfigV1::x2: hb = 2; break;
    case Pds::Quartz::ConfigV1::x4: hb = 4; break;
    default: hb = 0; break;
    }
    SetParameter("Horizontal Binning","HBIN",hb);
  }
  SetParameter("Output Mirroring","MI",_inputConfig->output_mirroring());
#if 0
  SetParameter("Vertical Remap"  ,"VR",_inputConfig->vertical_remapping());
#endif
  SetParameter("Output Resolution","OR",_inputConfig->output_resolution_bits());
  SetParameter("Pixel Clock Speed","CLC",2); // always choose 66MHz

  //
  //  Output format control
  //
  //     Number of taps: { 2, 4, 8, 10 }
  //     Q4A150 (66MHz) : 2 (30Hz OR8 or OR10  Base CameraLink), 
  //                      4 (60Hz OR8 or OR10  Medium CameraLink), 
  //                      8 (120Hz only OR8    Full CameraLink)
  //
  //     LVAL gap (clks) : 

  SetParameters("Output Format","OFRM",camera_taps(),4);

  setTestPattern( driver, false );
  //  setTestPattern( driver, true );

  if (_inputConfig->output_lookup_table_enabled()) {
    SetCommand("Output LUT Begin","OLUTBGN");
    const unsigned short* lut = _inputConfig->output_lookup_table();
    const unsigned short* end = lut + 4096;
    while( lut < end ) {
      snprintf(szCommand, SZCOMMAND_MAXLEN, "OLUT%hu", *lut++);
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
  SetCommand  ("Reset Frame Counter","FCR");
  LastCount = 0;

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
  SetParameter ("Request Mode","RQM",0);
#endif

  CurrentCount = RESET_COUNT;

  return 0;
}

int QuartzCamera::setTestPattern( CameraDriver& driver, bool on )
{
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];

  int ret;
  SetParameter( "Test Pattern", "TP", on ? 1 : 0 );
  
  return 0;
}

int QuartzCamera::setContinuousMode( CameraDriver& driver, double fps )
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

bool QuartzCamera::validate(Pds::FrameServerMsg& msg)
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

#if 1
  //  Allow single frame drops
  CurrentCount = Count;
  if (CurrentCount != msg.count+LastCount) {
    printf("Camera frame number(%d) != Server number(%d)\n",
           CurrentCount, msg.count+LastCount);
    LastCount++;
    if (CurrentCount != msg.count+LastCount) {
      return false;
    }
  }
  msg.count = CurrentCount;
#else
  if (CurrentCount==RESET_COUNT) {
    CurrentCount = 0;
    LastCount = Count;
  }
  else
    CurrentCount = Count - LastCount;

  if (CurrentCount != msg.count) {
    printf("Camera frame number(%d) != Server number(%d)\n",
           CurrentCount, msg.count);
    return false;
  }
#endif
  return true;
}

int           QuartzCamera::baudRate() const { return 57600; }
char          QuartzCamera::eotWrite() const { return 0x06; }
char          QuartzCamera::eotRead () const { return '\r'; }
char          QuartzCamera::sof     () const { return '@'; }
char          QuartzCamera::eof     () const { return '\r'; }
unsigned long QuartzCamera::timeout_ms() const { return 40; }

int  QuartzCamera::camera_width () const { return QuartzConfigType::max_column_pixels(_src); }
int  QuartzCamera::camera_height() const { return QuartzConfigType::max_row_pixels   (_src); }
int  QuartzCamera::camera_depth () const { return _inputConfig ? _inputConfig->output_resolution_bits() : 8; }
int  QuartzCamera::camera_taps  () const { 
  if (_inputConfig && 
      _inputConfig->output_resolution_bits()>8 &&
      MAX_TAPS>4)
    return 4;

  return MAX_TAPS;
}

const char* QuartzCamera::camera_name() const 
{
  switch(_src.device()) {
#if 0
  case DetInfo::Quartz4A150:
    switch(camera_taps()) {
    case 2:
      switch(camera_depth()) {
      case 10: return "Adimec_Quartz-4A150m/Q_F10bit_2tap";
      case  8: 
      default: return "Adimec_Quartz-4A150m/Q_F8bit_2tap";
    } break;
    case 4:
      switch(camera_depth()) {
      case 10: return "Adimec_Quartz-4A150m/Q_F10bit_4tap";
      case  8: 
      default: return "Adimec_Quartz-4A150m/Q_F8bit_4tap";
    } break;
    case 8:
    default:
      printf("QuartzCamera (%d taps) not defined for Leutron frame grabber\n",
             camera_taps());
      return 0;
    }
    break;
#endif
  default:
    printf("QuartzCamera::camera_name illegal type %s\n",
           DetInfo::name(_src.device()));
    exit(-1);
    break;
  }
  return 0;
}
