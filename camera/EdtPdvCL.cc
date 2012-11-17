#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include "pds/camera/CameraBase.hh"
#include "pds/camera/EdtPdvCL.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/Semaphore.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <time.h>

//  library routine declaration missing from edt header files
extern "C" {
  int  pdv_update_size(PdvDev *);
}

static void display_serialcmd(char*, int);
static int nPrint=0;
static const int _tmo_ms = 0x7fffffff;

namespace Pds {

  class EdtReaderEnable : public Routine {
  public:
    EdtReaderEnable(EdtReader* reader, 
		    bool v) :
      _reader(reader), 
      _sem   (Semaphore::EMPTY), 
      _v     (v) {}
    ~EdtReaderEnable() {}
  public:
    void routine();
    void unblock() { _sem.give(); }
    void block  () { _sem.take(); }
  private:
    EdtReader*     _reader;
    Semaphore _sem;
    bool           _v;
  };

  class EdtReader : public Routine {
  public:
    EdtReader(EdtPdvCL*  base,
	      Task* task) :
      _base(base),
      _task(task),
      _enable (new EdtReaderEnable(this,true )),
      _disable(new EdtReaderEnable(this,false)),
      _last_timeouts  (0),
      _recover_timeout(false),
      _running        (false)
    {
      _sleepTime.tv_sec = 0;
      _sleepTime.tv_nsec = (long) 0.5e9;  //0.5 sec
    }
    ~EdtReader() {}
  public:
    void start        () {} // { _task->call(this); }
    void queue_enable () { _task->call(_enable ); _enable ->block(); }
    void queue_disable() { _task->call(_disable); edt_do_timeout(_base->dev()); _disable->block(); }
    void enable       (bool v)
    {
      nPrint=0;
      PdvDev* pdv_p = _base->dev();
      _running = v; 
      if (v) {
        pdv_cl_reset_fv_counter(pdv_p);
	pdv_multibuf(_base->dev(), 4);
	_last_timeouts = pdv_timeouts(_base->dev());
	pdv_start_images(pdv_p, 4);
	_task->call(this);
	_enable->unblock();
      }
      else {
        edt_do_timeout(_base->dev());
      }
    }
    void routine()
    {
      PdvDev* pdv_p = _base->dev();

      u_char* image_p = pdv_wait_image(pdv_p);

      int overrun  = edt_reg_read(pdv_p, PDV_STAT) & PDV_OVERRUN;

      pdv_start_images(pdv_p, 1);

      int timeouts = pdv_timeouts(pdv_p);

      if (nPrint) {
        nPrint--;
        int nfv = pdv_cl_get_fv_counter(pdv_p);
        int curdone = edt_done_count(pdv_p);
        int curtodo = edt_get_todo(pdv_p);
        
        printf("EDT: nfv %d  tmo %d  done %d  todo %d\n",
               nfv, timeouts, curdone, curtodo);
      }

      if (timeouts > _last_timeouts) {
	pdv_timeout_restart(pdv_p, TRUE);
	_last_timeouts = timeouts;
	_recover_timeout = true;
      }
      else if (overrun) {
	printf("overrun...\n");
      }
      else {
	_base->handle(image_p);
      }

      if (_running)
	_task->call(this);
      else
	_disable->unblock();
    }
  private:
    EdtPdvCL*  _base;
    Task* _task;
    EdtReaderEnable* _enable;
    EdtReaderEnable* _disable;
    int       _last_timeouts;
    bool      _recover_timeout;
    bool      _running;
    timespec  _sleepTime;
  };
};

using namespace Pds;

void EdtReaderEnable::routine() { 
  _reader->enable(_v); 
}

EdtPdvCL::EdtPdvCL(CameraBase& camera, int unit, int channel) :
  CameraDriver(camera),
  _unit    (unit),
  _channel (channel),
  _dev     (0),
  _acq     (new EdtReader(this, new Task(TaskObject("edtacq")))),
  _fsrv    (0),
  _app     (0),
  _occPool (new GenericPool(sizeof(Occurrence),4)),
  _hsignal ("CamSignal")
{

#ifdef DBUG
  _tsignal.tv_sec = _tsignal.tv_nsec = 0;
#endif

  _sample_sec  = 0;
  _sample_nsec = 0;
}

EdtPdvCL::~EdtPdvCL()
{
}


void EdtPdvCL::_setup(int unit, int channel)
{
  CameraBase& c = camera();

#if 1
  //
  // EDT initialization
  //   (taken from initcam.c)
  //
  Dependent* dd_p = pdv_alloc_dependent();
  if (dd_p == NULL) {
    printf("Opal1kEdt::alloc_dependent failed\n");
    exit(1);
  }

  Edtinfo edtinfo;
  Edtinfo* ei_p = &edtinfo;

  { 
    memset(dd_p, 0, sizeof(Dependent));
    dd_p->rbtfile[0] = '\0';
    dd_p->cameratype[0] = '\0';
    dd_p->shutter_speed = NOT_SET;
    dd_p->default_shutter_speed = NOT_SET;
    dd_p->default_gain = NOT_SET;
    dd_p->default_offset = NOT_SET;
    dd_p->default_aperture = NOT_SET;
    dd_p->binx = 1;
    dd_p->biny = 1;
    dd_p->byteswap = NOT_SET;
    dd_p->serial_timeout = 1000;
    dd_p->serial_response[0] = '\r';
    dd_p->xilinx_rev = NOT_SET;
    dd_p->timeout = NOT_SET;
    dd_p->user_timeout = _tmo_ms;  // set get_images timeout to be more responsive
    dd_p->mode_cntl_norm = NOT_SET;
    dd_p->mc4 = 0;
    dd_p->pulnix = 0;
    dd_p->dbl_trig = 0;
    dd_p->shift = NOT_SET;
    dd_p->mask = 0xffff;
    dd_p->mode16 = NOT_SET;
    dd_p->serial_baud = NOT_SET;
    dd_p->serial_waitc = NOT_SET ;
    dd_p->serial_format = SERIAL_ASCII;
    strcpy(dd_p->serial_term, "\r");	/* term for most ASCII exc. ES4.0 */
    
    dd_p->kbs_red_row_first = 1;
    dd_p->kbs_green_pixel_first = 0;
  

    dd_p->htaps = NOT_SET;
    dd_p->vtaps = NOT_SET;

    dd_p->cameralink = 0;
    dd_p->start_delay = 0;
    dd_p->frame_period = NOT_SET;
    dd_p->frame_timing = NOT_SET;

    dd_p->strobe_enabled = NOT_SET;
    dd_p->register_wrap = 0;
    dd_p->serial_init_delay = NOT_SET;

    /*
     * xregwrite registers can be 0-ff. We need a way to flag the
     * end of the array, so just waste ff and call that "not set"
     */
    for (int i=0; i<32; i++)
	dd_p->xilinx_flag[i] = 0xff;
  }

  ei_p->startdma = NOT_SET;
  ei_p->enddma = NOT_SET;
  ei_p->flushdma = NOT_SET;

  dd_p->startdma = NOT_SET;
  dd_p->enddma = NOT_SET;
  dd_p->flushdma = NOT_SET;

  strcpy(dd_p->cfgname, "EdtPdvCL.cc");

  strcpy(dd_p->camera_class, "Generic");
  strcpy(dd_p->camera_model, "Camera Link LCLS DAQ");
  strcpy(dd_p->camera_info,  "1024x1024 (pulsew mode)");

  dd_p->width  = c.camera_width();
  dd_p->height = c.camera_height();
  dd_p->depth  = c.camera_depth();
  dd_p->extdepth = c.camera_depth();

  dd_p->cameralink = 1;
  dd_p->cl_data_path = (c.camera_depth()-1) | ((c.camera_taps()-1)<<4);
  dd_p->cl_cfg       = 0x02;
  dd_p->htaps        = c.camera_taps();

  dd_p->vtaps        = NOT_SET;
  //  dd_p->swinterlace  = PDV_WORD_INTLV;
  dd_p->camera_shutter_timing  = AIA_SERIAL;

  sprintf(dd_p->cameratype, "%s %s %s",
	  dd_p->camera_class,
	  dd_p->camera_model,
	  dd_p->camera_info);

  if (dd_p->cl_data_path && dd_p->cls.taps == 0)
    dd_p->cls.taps = ((dd_p->cl_data_path >> 4) & 0xf) + 1;

  char unitstr[8];
  sprintf(unitstr,"%d_%d",unit,channel);

  char edt_devname[256];
  strcpy(edt_devname, EDT_INTERFACE);

  printf("Opening %s %d %d\n",edt_devname,unit,channel);

  EdtDev* edt_p = edt_open_channel(edt_devname, unit, channel);
  if (edt_p == NULL) {
    char errstr[64];
    printf(errstr, "edt_open(%s%d)", edt_devname, unit);
    edt_perror(errstr);
    exit(1);
  }

  if (pdv_initcam(edt_p, dd_p, unit, ei_p, "", "", 0) != 0) {
    printf("EdtPdvCL::initcam failed\n");
    edt_close(edt_p);
    exit(1);
  }
#else
  EdtDev* edt_p = pdv_open_channel(EDT_INTERFACE, unit, channel);
  if (!edt_p) {
    char errstr[64];
    printf(errstr, "edt_open(%s%d)", EDT_INTERFACE, unit);
    edt_perror(errstr);
    exit(1);
  }      
#endif

  _dev = edt_p;
}

int EdtPdvCL::initialize(UserMessage* msg)
{
  CameraBase& c = camera();

  _setup(_unit,_channel);

  pdv_set_baud(_dev, c.baudRate());

  char writeBuffer[64];
  memset(writeBuffer,0,64);
  int n = pdv_serial_read(_dev, writeBuffer, 64);
  display_serialcmd(writeBuffer, n);

  _nposts = 0;

  int ret = c.configure(*this,msg);

  Dependent* dd_p = _dev->dd_p;
  dd_p->width  = c.camera_width();
  dd_p->height = c.camera_height();
  dd_p->depth  = c.camera_depth();
  dd_p->extdepth = c.camera_depth();

  // The following call doesn't seem to work properly.
  // Changes in the camera bit depth cause timeouts
  // in the readout afterwards.
  pdv_update_size(_dev); 

  _acq->start();

  return ret;
}

int EdtPdvCL::start_acquisition(FrameServer& fsrv,  // post frames here
				Appliance  & app,   // post occurrences here
				UserMessage* msg)
{
  _outOfOrder = false;
  _fsrv = &fsrv;
  _app  = &app;

  _acq->queue_enable();

  return 0;
}

int EdtPdvCL::stop_acquisition()
{
  _acq->queue_disable();
  _fsrv->clear();

  pdv_close(_dev);

  _fsrv = 0;
  _app  = 0;
  return 0;
}

//
//  Strange behavior where channel 1 always times out.
//  Need to keep timeouts short to keep the system responsive.
//
int EdtPdvCL::SendCommand(char *szCommand, 
			  char *pszResponse, 
			  int  iResponseBufferSize)
{
  CameraBase& c = camera();

  const int BUFFER_SIZE=64;
  char* write_buffer = new char[BUFFER_SIZE];
  if (c.uses_sof())
    sprintf(write_buffer,"%c%s%c",c.sof(),szCommand,c.eof());
  else
    sprintf(write_buffer,"%s%c",szCommand,c.eof());
  //  sprintf(write_buffer,"%c%s",c.sof(),szCommand);

  int r,n;
  n = pdv_serial_command(_dev,write_buffer);
  if (n < 0) {
    printf("pdv_serial_command failed\n");
    return 0;
  }

  if (pszResponse) {
    pdv_set_waitchar(_dev,1,c.eotRead());
    n = pdv_serial_wait(_dev, c.timeout_ms(), BUFFER_SIZE);
    r = pdv_serial_read(_dev, pszResponse, BUFFER_SIZE);
  }
  else {
    pdv_set_waitchar(_dev,1,c.eotWrite());
    n = pdv_serial_wait(_dev, c.timeout_ms(), BUFFER_SIZE);
    r = pdv_serial_read(_dev, write_buffer, BUFFER_SIZE);
  }
  return r;
}

int EdtPdvCL::SendBinary (char *szCommand, 
			  int  iCommandLen,
			  char *pszResponse, 
			  int  iResponseBufferSize)
{
  printf("this command untested\n");

  CameraBase& c = camera();

  const int BUFFER_SIZE=64;
  char* write_buffer = new char[BUFFER_SIZE];
  if (c.uses_sof())
    sprintf(write_buffer,"%c%s%c",c.sof(),szCommand,c.eof());
  else
    sprintf(write_buffer,"%s%c",szCommand,c.eof());
  pdv_serial_command(_dev,write_buffer);

  pdv_set_waitchar(_dev,1,c.eotWrite());
  int n = pdv_serial_wait(_dev, c.timeout_ms(), BUFFER_SIZE);
  int r = pdv_serial_read(_dev, write_buffer, BUFFER_SIZE);

  if (pszResponse) {
    pdv_set_waitchar(_dev,1,c.eotRead());
    n = pdv_serial_wait(_dev, c.timeout_ms(), iResponseBufferSize);
    r = pdv_serial_read(_dev, pszResponse , iResponseBufferSize);
  }

  return r;
}

void EdtPdvCL::dump()
{
}

void EdtPdvCL::handle(u_char* image_p)
{
#ifdef DBUG
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  if (_tsignal.tv_sec)
    _hsignal.accumulate(ts,_tsignal);
  _tsignal=ts;
#endif

  CameraBase& c = camera();
  FrameServerMsg* msg =
    new FrameServerMsg(FrameServerMsg::NewFrame,
                            image_p,
                            c.camera_width(),
                            c.camera_height(),
                            c.camera_depth(),
                            _nposts,
                            0);

  //  Test out-of-order
  if (_outOfOrder)
    msg->damage.increase(Damage::OutOfOrder);

  else if (!c.validate(*msg)) {
    _outOfOrder = true;

    Occurrence* occ = new (_occPool)
      Occurrence(OccurrenceId::ClearReadout);
    _app->post(occ);

    UserMessage* umsg = new (_occPool)
      UserMessage;
    umsg->append("Frame readout error\n");
    umsg->append(DetInfo::name(static_cast<const DetInfo&>(_fsrv->client())));
    _app->post(umsg);

    msg->damage.increase(Damage::OutOfOrder);
  }

  _fsrv->post(msg);
  _nposts++;

#ifdef DBUG
  _hsignal.print(_nposts);
#endif
}

void display_serialcmd(char* cmd, int len)
{
  int i;
  for(i=0; i<len; i++) {
    char *str;
    switch (cmd[i]) {
    case 0: str="\\0"; break;
    case '\r': str="<CR>"; break;
    case 0x02: str="<STX>"; break;
    case 0x03: str="<ETX>"; break;
    case 0x06: str="<ACK>"; break;
    case 0x15: str="<NAK>"; break;
    default: 
      char str_data[2];
      str=str_data;
      str[0]=cmd[i];
      str[1]=0;
      break;
    };
    printf(str);
  }
  printf("<END>\n");
}
