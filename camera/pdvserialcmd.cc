//! serialcmd.cc
//! This tool allows you to send and receive data through
//! the camera link serial port of a Leutron frame grabber
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!
//
//  An example looks like this:
//
// build/pds/bin/i386-linux-opt/serialcmd --camera Pulnix_TM6740CL_8bit --grabber "PicPortX CL Mono" --baudrate 9600
// build/pds/bin/i386-linux-opt/serialcmd --camera Adimec_Opal-1000m/Q_F8bit --grabber "PicPortX CL Mono PMC" --baudrate 115200
//

#include "pds/service/CmdLineTools.hh"
#include <stdio.h>
#include <stdlib.h>
#include <edtinc.h>


#define RECEIVEBUFFER_SIZE  64
#define RECEIVE_TIMEOUT_MS  1000

static void help(const char* p)
{
  printf("Usage: %s [--baud <baudrate>] [--unit <unit>] [--channel <channel>]\n",p);
}

static EdtDev* _setup(int unit, int channel)
{
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
    dd_p->user_timeout = NOT_SET;
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
  ei_p->serial_init = 0;

  dd_p->startdma = NOT_SET;
  dd_p->enddma = NOT_SET;
  dd_p->flushdma = NOT_SET;

  strcpy(dd_p->cfgname, "EdtPdvCL.cc");

  strcpy(dd_p->camera_class, "Generic");
  strcpy(dd_p->camera_model, "Camera Link LCLS DAQ");
  strcpy(dd_p->camera_info,  "1024x1024 (pulsew mode)");

  dd_p->width  = 1024;
  dd_p->height = 1024;
  dd_p->depth  = 12;
  dd_p->extdepth = 12;

  dd_p->cameralink = 1;
  dd_p->cl_data_path = (12-1) | ((2-1)<<4);
  dd_p->cl_cfg       = 0x02;
  dd_p->htaps        = 2;

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

  char no_string = '\0';
  if (pdv_initcam(edt_p, dd_p, unit, ei_p, &no_string, &no_string, 0) != 0) {
    printf("EdtPdvCL::initcam failed\n");
    edt_close(edt_p);
    exit(1);
  }

  return edt_p;
}

int display_serialcmd(char *cmd, int len)
{   int i;
    for(i=0; i<len; i++) {
        const char *str;
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
            str_data[0]=cmd[i];
            str_data[1]=0;
            break;
        };
        printf(str);
    }
    printf("<END>\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char          eotWrite = 0x06; // end-of-transfer from camera after write
    char          eotRead  = '\r'; // end-of-transfer from camera after read
    //    char          sof      = '@';
    char          eof      = '\r';

    int i;
    char pReceiveBuffer[RECEIVEBUFFER_SIZE];
    memset(pReceiveBuffer,0,RECEIVEBUFFER_SIZE);

    int baud = 9600;
    int unit    = 0;
    int channel = 0;

    bool parse_valid = true;

    // Parse arguments
    for(i=1; i<argc; i++) {
        if ((strcmp(argv[i], "--help") == 0) ||
            (strcmp(argv[i], "-h") == 0)) {
            help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--baud") == 0) {
          parse_valid &= Pds::CmdLineTools::parseInt(argv[++i],baud);
        } else if (strcmp(argv[i], "--unit") == 0) {
          parse_valid &= Pds::CmdLineTools::parseInt(argv[++i],unit);
        } else if (strcmp(argv[i], "--channel") == 0) {
          parse_valid &= Pds::CmdLineTools::parseInt(argv[++i],channel);
        } else {
            printf("ERROR: invalid argument %s, try --help.\n\n", argv[i]);
            return 1;
        }
    }

    //    parse_valid &= (optind==argc);

    if (!parse_valid) {
      help(argv[0]);
      return -1;
    }

#if 1     // Assume Opal1k
    EdtDev* pdv_p = _setup(unit, channel);
#else
    EdtDev* pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel);
    if (!pdv_p) {
      pdv_perror(EDT_INTERFACE);
      return -1;
    }
#endif
    pdv_set_baud(pdv_p, baud);

    int n;

    //  flush buffers
    n = pdv_serial_read(pdv_p, pReceiveBuffer, RECEIVEBUFFER_SIZE);
    display_serialcmd(pReceiveBuffer, n);

    do {
      const int maxlen=128;
      char line[maxlen];
      printf("Enter command: ");
      char* result = fgets(line, maxlen, stdin);

      if (!result) break;

      result[strlen(result)-1] = eof;

      display_serialcmd(result, strlen(result));

      // Send command
      n = pdv_serial_command(pdv_p,result);
      if (n < 0) {
	printf("pdv_serial_command failed\n");
      }
      
      int r;
      if (result[strlen(result)-2] == '?') {
        pdv_set_waitchar(pdv_p,1,eotRead);
        n = pdv_serial_wait(pdv_p, RECEIVE_TIMEOUT_MS, RECEIVEBUFFER_SIZE);
        r = pdv_serial_read(pdv_p, pReceiveBuffer, RECEIVEBUFFER_SIZE);
      }
      else {
        pdv_set_waitchar(pdv_p,1,eotWrite);
        n = pdv_serial_wait(pdv_p, RECEIVE_TIMEOUT_MS, RECEIVEBUFFER_SIZE);
        r = pdv_serial_read(pdv_p, pReceiveBuffer, RECEIVEBUFFER_SIZE);
      }
      printf("pdv_serial_read returned %d (%d)\n",r,n);
      display_serialcmd(pReceiveBuffer, r);
      printf("\n\n");

      
    } while(1);
        
    // Close everything
    pdv_close(pdv_p);
    return 0;
}
