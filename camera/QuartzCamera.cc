#include "pds/camera/QuartzCamera.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/camera/AdimecCommander.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define DBUG

#define RESET_COUNT 0x80000000

// Base mode
//#define MAX_TAPS 2
// Medium mode
//#define MAX_TAPS 4
// Full mode
#define MAX_TAPS 8

using namespace Pds;

QuartzCamera::QuartzCamera(const DetInfo& src) :
  _src        (src),
  _inputConfig(0),
  _max_taps   (MAX_TAPS)
{
}

QuartzCamera::QuartzCamera(const DetInfo& src,
                           CLMode mode) :
  _src        (src),
  _inputConfig(0)
{
  switch(mode) {
  case Base  : _max_taps=2; break;
  case Medium: _max_taps=4; break;
  case Full  : _max_taps=8; break;
  default    : _max_taps=MAX_TAPS; break;
  }
}

QuartzCamera::~QuartzCamera()
{
}

void QuartzCamera::set_config_data(const void* p)
{
  _inputConfig = reinterpret_cast<const QuartzConfigType*>(p);
  _max_taps = _inputConfig->max_taps();
}

#define SetCommand(title,op)    cmd.setCommand(title,op)

#define GetParameter(op,rval)   cmd.getParameter(op,rval)

#define GetParameters(op,val,n) cmd.getParameters(op,val,n)

#define SetParameterA(op,val) {            \
    int ret=cmd.setParameter(op,val);      \
    if (ret<0) return ret; }

#define SetParameter(title,op,val) {            \
    int ret=cmd.setParameter(title,op,val);     \
    if (ret<0) return ret; }

#define SetParameters(title,op,val,n) {            \
    cmd.setParameter(title,op,val,n);              \
  }

#define SetParametersQ(title,op,val,n) {          \
    int ret=cmd.setParameter(title,op,val,n);     \
    if (ret<0) return ret; }

int QuartzCamera::configure(CameraDriver& driver,
			    UserMessage*  msg)
{
  AdimecCommander cmd(driver);

  unsigned val[4];
  char *versionString;

  QuartzConfigType* outputConfig = const_cast<QuartzConfigType*>(_inputConfig);

  GetParameter("BIT", val[0]);
  printf( ">> Built-In Self Test (BIT): '%d' %s \n", val[0],
          ( val[0] == 0) ? "[OK]" : "[FAIL]" );
  
  GetParameter("TM", val[0] );
  printf( ">> Camera temperature (TM): '%d' C\n", val[0] );

#if 0
  GetParameters( "ET", val, 2);
  printf( ">> Total on time: '%d' * 65536 + '%d' = %d hours\n", val[0], val[1],
          val[0] * 65536 + val[1] );

  GetParameter( "UFDT", val[0] );
  printf( ">> Microcontroller firmware release: '%s'\n", cmd.response() );
#endif

  GetParameter( "BS", val[0] );
  printf( ">> Build versions: '%s'\n", cmd.response() );

  // FPGA firmware versions 1.20 and newer have the external
  // trigger polarity inverted.  If FPGA firmware is 1.20 or later,
  // we compensate by inverting the polarity requested by software.

  // Command syntax: BS?
  // Reply message: "x.xx;y.yy;z.zz
  //   Where x.xx stands for camera issue, y.yy indicates the microcontroller firmware version
  //   and z.zz indicates the FPGA firmware version.
  if (((versionString = strchr(cmd.response(), ';')) != NULL) &&
      ((versionString = strchr(versionString + 1, ';')) != NULL)) {
    // advance to character after the 2nd ';'
    ++versionString;
  }
  printf("versionString: %s\n",versionString);

  GetParameter( "ID", val[0] );
  printf( ">> Camera ID: '%s'\n", cmd.response() );
  
#if 0
  GetParameter( "MID", val[0] );
  printf( ">> Camera Model ID: '%s'\n", cmd.response() );
#endif
  
  GetParameter( "SN", val[0] );
  printf( ">> Camera Serial #: '%s'\n", cmd.response() );

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
  SetParameter("Output Resolution","OR",_inputConfig->output_resolution_bits());
  SetParameter("Pixel Clock Speed","CLC",2); // always choose 66MHz

  unsigned roi[4];
  if (_inputConfig->use_hardware_roi()) {
    roi[0] = _inputConfig->roi_lo().column();
    roi[1] = _inputConfig->roi_lo().row();
    roi[2] = _inputConfig->roi_hi().column()-_inputConfig->roi_lo().column()+1;
    roi[3] = _inputConfig->roi_hi().row   ()-_inputConfig->roi_lo().row   ()+1;
    SetParametersQ("Hardware ROI","ROI",roi,4);
    *new(outputConfig) QuartzConfigType(_inputConfig->output_offset(),
                                        _inputConfig->gain_percent(),
                                        _inputConfig->output_resolution(),
                                        _inputConfig->horizontal_binning(),
                                        _inputConfig->vertical_binning(),
                                        _inputConfig->output_mirroring(),
                                        _inputConfig->output_lookup_table_enabled(),
                                        _inputConfig->defect_pixel_correction_enabled(),
                                        _inputConfig->use_hardware_roi(),
                                        _inputConfig->use_test_pattern()||_useTestPattern(),
                                        _inputConfig->max_taps(),
                                        Camera::FrameCoord(roi[0],roi[1]),
                                        Camera::FrameCoord(roi[0]+roi[2]-1,roi[1]+roi[3]-1),
                                        0,
                                        _inputConfig->output_lookup_table().data(),
                                        0);
  }
  else {
    roi[0] = 0;
    roi[1] = 0;
    roi[2] = 2048;
    roi[3] = 2048;
    SetParametersQ("Hardware ROI","ROI",roi,4);
  }

  //
  //  Output format control
  //
  //     Number of taps: { 2, 4, 8, 10 }
  //     Q4A150 (66MHz) : 2 (30Hz OR8 or OR10  Base CameraLink), 
  //                      4 (60Hz OR8 or OR10  Medium CameraLink), 
  //                      8 (120Hz only OR8    Full CameraLink)
  //
  //     LVAL gap (clks) : 

  unsigned lval_gap;
  switch(camera_taps()) {
  case 2:  lval_gap = 128; break;
  case 8:
  default: lval_gap =   4; break;
  }

  val[0]=camera_taps();
  val[1]=lval_gap;
  SetParameters("Output Format","OFRM",val,2);

  // setTestPattern( driver, false );
  setTestPattern( driver, 
                  _inputConfig->use_test_pattern() || _useTestPattern() );

  if (_inputConfig->output_lookup_table_enabled()) {
    cmd.setCommand("Output LUT Begin","OLUTBGN");
    SetParameterA("OLUT%hu",_inputConfig->output_lookup_table());
    cmd.setCommand("Output LUT Begin","OLUTEND");
    SetParameter("Output LUT Enabled","OLUTE",1);
  }
  else
    SetParameter("Output LUT Enabled","OLUTE",0);

  if (_inputConfig->defect_pixel_correction_enabled()) {
    //  read defect pixels into output config
    unsigned n;
    char cmdb[8];
    GetParameter("DP0",n);

    if (n) {
      Pds::Camera::FrameCoord* dps = new Pds::Camera::FrameCoord[n];
      
      for(unsigned k=0; k<n; k++) {
        sprintf(cmdb,"DP%d",k+1);
        GetParameters(cmdb,val,2);
        dps[k] = Pds::Camera::FrameCoord(val[0],val[1]);
      }

      SetParameter("Defect Pixel Correction","DPE",1);
    
      *new(outputConfig) QuartzConfigType(_inputConfig->output_offset(),
                                          _inputConfig->gain_percent(),
                                          _inputConfig->output_resolution(),
                                          _inputConfig->horizontal_binning(),
                                          _inputConfig->vertical_binning(),
                                          _inputConfig->output_mirroring(),
                                          _inputConfig->output_lookup_table_enabled(),
                                          _inputConfig->defect_pixel_correction_enabled(),
                                          _inputConfig->use_hardware_roi(),
                                          _inputConfig->use_test_pattern()||_useTestPattern(),
                                          _inputConfig->max_taps(),
                                          _inputConfig->roi_lo(),
                                          _inputConfig->roi_hi(),
                                          n,
                                          _inputConfig->output_lookup_table().data(),
                                          dps);
      delete[] dps;
    }
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
	        1000000/(unsigned long)config.FramesPerSec);
  SetParameter ("Integration Time","IT",
	        config.ShutterMicroSec/10);
#else
  SetParameter ("Operating Mode","MO",1);
  val[0]=4;
  val[1]=1;
  SetParametersQ("External Inputs","CCE",val,2);
  SetParameter ("Request Mode","RQM",0);
#endif

  return 0;
}

int QuartzCamera::setTestPattern( CameraDriver& driver, bool on )
{
  AdimecCommander cmd(driver);

  SetParameter( "Test Pattern", "TP", on ? 1 : 0 );
  
  return 0;
}

int QuartzCamera::setContinuousMode( CameraDriver& driver, double fps )
{
  AdimecCommander cmd(driver);

  unsigned _mode=0, _fps=unsigned(1000000/fps), _it=_fps-100;
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

  if (Count != msg.count) {
    //  Missing frame will be detected by eventbuilder
    // return false;   
  }

  msg.count  = Count;
  msg.offset = _inputConfig->output_offset();
  return true;
}

int           QuartzCamera::baudRate() const { return 57600; }
char          QuartzCamera::eotWrite() const { return 0x06; }
char          QuartzCamera::eotRead () const { return '\r'; }
char          QuartzCamera::sof     () const { return '@'; }
char          QuartzCamera::eof     () const { return '\r'; }
unsigned long QuartzCamera::timeout_ms() const { return 40; }

int  QuartzCamera::camera_width () const 
{ 
  int v = Pds::Quartz::max_column_pixels(_src); 
  if (_inputConfig) {
    if (_inputConfig->use_hardware_roi())
      v = (_inputConfig->roi_hi().column()-_inputConfig->roi_lo().column()+1);
    switch(_inputConfig->horizontal_binning()) {
    case QuartzConfigType::x2: v/=2; break;
    case QuartzConfigType::x4: v/=4; break;
    default: break;
    }
  }
  return v;
}

int  QuartzCamera::camera_height() const 
{
  int v = Pds::Quartz::max_row_pixels(_src); 
  if (_inputConfig) {
    if (_inputConfig->use_hardware_roi())
      v = (_inputConfig->roi_hi().row()-_inputConfig->roi_lo().row()+1);
    switch(_inputConfig->vertical_binning()) {
    case QuartzConfigType::x2: v/=2; break;
    case QuartzConfigType::x4: v/=4; break;
    default: break;
    }
  }
  return v;
}

int  QuartzCamera::camera_depth () const { return _inputConfig ? _inputConfig->output_resolution_bits() : 8; }
int  QuartzCamera::camera_taps  () const { 
  if (_inputConfig && 
      _inputConfig->output_resolution_bits()>8 &&
      _max_taps>4)
    return 4;

  return _max_taps;
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
