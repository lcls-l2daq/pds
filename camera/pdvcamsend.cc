//! camera_test.cc
//! This program test and demonstrate how to stream data from a
//! camera using the Camera class.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include "camstream.h"
#include "pthread.h"

#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FccdCamera.hh"
#include "pds/camera/EdtPdvCL.hh"

#include <edtinc.h>

#define USE_TCP

#define OPALGAIN_MIN      1.
#define OPALGAIN_MAX      32.
#define OPALGAIN_DEFAULT  30.

#define OPALEXP_MIN       0.01
#define OPALEXP_MAX       320.00

using namespace Pds;

static unsigned long long gettime(void)
{	struct timeval t;

	gettimeofday(&t, NULL);
	return t.tv_sec*1000000+t.tv_usec;
}

static void help(const char *progname)
{
  printf("%s: stream data from a Camera object to the network.\n"
    "Syntax: %s <dest> [ OPTIONS ] | --help\n"
    "<dest>: stream processed frames to the specified\n"
    "\taddress with format <host>[:<port>].\n"
    "--count <nimages>: number of images to acquire.\n"
    "--fps <nframes>: frames per second.\n"
    "--shutter: use external shutter.\n"
    "--camera  {0|1|2}   : choose Opal (0) or Pulnix (1) or FCCD (2).\n"
    "--grabber <grabber#>: choose grabber number\n"
    "--noccd: enable data module test pattern (FCCD only).\n"
    "--opalgain: digital gain (%4.2f - %5.2f, default %5.2f) (Opal only).\n"
    "--opalexp: integration time (%4.2f - %5.2f ms, default frame period minus 0.1) (Opal only).\n"
    "--help: display this message.\n"
    "\n", progname, progname, OPALGAIN_MIN, OPALGAIN_MAX, OPALGAIN_DEFAULT,
                              OPALEXP_MIN, OPALEXP_MAX);
  return;
}


int main(int argc, char *argv[])
{ // int exttrigger = 0;
  int extshutter = 0;
  int noccd = 0;
  double fps = 0;
  int ifps =0;
  double opalgain = OPALGAIN_DEFAULT;
  double opalexp = -1;    // default: calculate based on frame rate
  unsigned uopalgain100 = (unsigned) int(opalgain * 100.);
  int i, ret, sockfd=-1;
  long nimages = 0;
  unsigned long long start_time, end_time;
  int camera_choice = 0;
  char *dest_host = NULL;
  char *strport;
  int  grabberid  = 0;
  int dest_port   = CAMSTREAM_DEFAULT_PORT;
  struct sockaddr_in dest_addr;
  struct hostent *dest_info;
  int bitsperpixel = 8;
  bool test_pattern;

  test_pattern = false;

  /* Parse the command line */
  for (i = 1; i < argc; i++) {
    if (strcmp("--help",argv[i]) == 0) {
      help(argv[0]);
      return 0;
    } else if (strcmp("--camera",argv[i]) == 0) {
      camera_choice = atoi(argv[++i]);
    } else if (strcmp("--shutter",argv[i]) == 0) {
      extshutter = 1;
    } else if (strcmp("--noccd",argv[i]) == 0) {
      noccd = 1;
    } else if( strcmp("--grabber", argv[i]) == 0 ) {
      grabberid = atoi(argv[++i]);
   	  printf( "Use grabber %d\n", grabberid );
      printf( "\n" );
    } else if( strcmp("--testpat", argv[i]) == 0 ) {
       // Currently only valid for the Opal1K camera!
       test_pattern = true;
    } else if (strcmp("--fps",argv[i]) == 0) {
      fps = strtod(argv[++i],NULL);
      ifps = int(fps);
    } else if (strcmp("--opalgain",argv[i]) == 0) {
      double dtmp = strtod(argv[++i],NULL);
      if ((dtmp >= OPALGAIN_MIN) && (dtmp <= OPALGAIN_MAX)) {
        opalgain = dtmp;
        uopalgain100 = (unsigned) int(opalgain * 100.);
      } else {
        fprintf(stderr, "ERROR: opalgain value %s out of range, try --help.\n",
            argv[i]);
        return -1;
      }
    } else if (strcmp("--opalexp",argv[i]) == 0) {
      double dtmp = strtod(argv[++i],NULL);
      if ((dtmp >= OPALEXP_MIN) && (dtmp <= OPALEXP_MAX)) {
        opalexp = dtmp;
      } else {
        fprintf(stderr, "ERROR: opalexp value %s out of range, try --help.\n",
            argv[i]);
        return -1;
      }
    } else if (strcmp("--count",argv[i]) == 0) {
      nimages = atol(argv[++i]);
    } else if (strcmp("--bpp",argv[i]) == 0) {
      bitsperpixel = atol(argv[++i]);
    } else if (dest_host == NULL) {
      dest_host = argv[i];
    } else {
      fprintf(stderr, "ERROR: Invalid argument %s, try --help.\n", 
          argv[i]);
      return -1;
    }
  }
  if((fps == 0) && (!extshutter)) {
    fprintf(stderr, "ERROR: set the frames per second (--fps) or the external shutter (--shutter).\n");
    return -1;
  }

  int level = 0;
  // verbose > 0
  level |= EDTAPP_MSG_INFO_1;
  level |= PDVLIB_MSG_INFO_1;
  level |= PDVLIB_MSG_WARNING;
  level |= PDVLIB_MSG_FATAL;
  // verbose > 1
  level |= EDTAPP_MSG_INFO_2;
  // verbose > 2
  level |= PDVLIB_MSG_INFO_2;

  edt_msg_set_level(edt_msg_default_handle(), level);

  printf( "Preparing camera...\n" );

  /* Open the camera */
  CameraBase* pCamera(NULL);

  switch(camera_choice) {
  case 0:
    {
      DetInfo info(0,DetInfo::NoDetector,0,DetInfo::Opal1000,0);
      Opal1kCamera* oCamera = new Opal1kCamera(info);
      Opal1kConfigType* Config = new Opal1kConfigType( 32, 100, 
						       bitsperpixel==8 ? Opal1kConfigType::Eight_bit : 
						       bitsperpixel==10 ? Opal1kConfigType::Ten_bit :
						       Opal1kConfigType::Twelve_bit,
						       Opal1kConfigType::x1,
						       Opal1kConfigType::None,
						       true, false);
      
      oCamera->set_config_data(Config);
      pCamera = oCamera;
      break;
    }
  case 2:
    {
      FccdCamera* fCamera = new FccdCamera;
      FccdConfigType* Config = new FccdConfigType
                                    (
                                    0,      // outputMode: 0 == FIFO
                                    noccd ? false : true,   // CCD Enable
                                    ifps ? true : false,    // Focus Mode Enable
                                    1,      // internal exposure time (ms)
                                    6.0,    // DAC 1
                                   -2.0,    // DAC 2
                                    6.0,    // DAC 3
                                   -2.0,    // DAC 4
                                    8.0,    // DAC 5
                                   -3.0,    // DAC 6
                                    5.0,    // DAC 7
                                   -5.0,    // DAC 8
                                    0.0,    // DAC 9
                                   -6.0,    // DAC 10
                                   50.0,    // DAC 11
                                    3.0,    // DAC 12
                                  -14.8,    // DAC 13
                                  -24.0,    // DAC 14
                                    0.0,    // DAC 15
                                    0.0,    // DAC 16
                                    0.0,    // DAC 17
                                    0xfe3f, // waveform0
                                    0x0380, // waveform1
                                    0xf8ff, // waveform2
                                    0x0004, // waveform3
                                    0xf83f, // waveform4
                                    0xf1ff, // waveform5
                                    0x1c00, // waveform6
                                    0xc7ff, // waveform7
                                    0x83ff, // waveform8
                                    0xfe3f, // waveform9
                                    0x0380, // waveform10
                                    0xf8ff, // waveform11
                                    0x0001, // waveform12
                                    0xf83f, // waveform13
                                    0x0020  // waveform14
                                    );
      fCamera->set_config_data(Config);
      pCamera = fCamera;
      break;
    }
  case 1:
  default:
    {
      TM6740Camera* tCamera = new TM6740Camera;
      TM6740ConfigType* Config = new TM6740ConfigType( 0x28,  // black-level-a
						       0x28,  // black-level-b
						       0xde,  // gain-tap-a
						       0xe9,  // gain-tap-b
						       false, // gain-balance
						       TM6740ConfigType::Eight_bit,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::Linear );
      tCamera->set_config_data(Config);
      pCamera = tCamera;
      break;
    }
  }

  EdtPdvCL* pDriver = new EdtPdvCL( *pCamera, grabberid, 0 );

  /* Configure the camera */
  printf("Configuring camera ... "); 
  fflush(stdout);
  
  Pds::GenericPool* pool = new Pds::GenericPool(sizeof(Pds::UserMessage),1);
  Pds::UserMessage* msg = new (pool)Pds::UserMessage;

  ret = pDriver->initialize(msg);
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::initialize: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }

  if( camera_choice == 0 ) {
    Opal1kCamera* opal = static_cast<Opal1kCamera*>(pCamera);
    opal->setTestPattern( *pDriver, test_pattern );
     if (!extshutter) {
       opal->setContinuousMode( *pDriver, fps);
     }
  }

  printf("done.\n"); 

  if (dest_host) {
    /* Initialize the socket */
    strport = strchr(dest_host, ':');
    if(strport != NULL) {
      *strport = 0;
      strport++;
      dest_port = atoi(strport);
    }
    /* Check the destination field and fill the address struct */
    dest_info = gethostbyname2(dest_host, AF_INET);
    if (dest_info == NULL) {
      fprintf(stderr, "\nSTREAMING ERROR: could not find %s: %s.\n", 
	      dest_host, hstrerror(h_errno));
      return -1;
    }
    dest_addr.sin_family = AF_INET;
    memcpy(&dest_addr.sin_addr.s_addr, dest_info->h_addr_list[0],
	   dest_info->h_length);
    dest_addr.sin_port = htons(dest_port);
    /* Open the socket and connection */
#ifdef USE_TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#else
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (sockfd < 0) {
      perror("STREAMING ERROR: socket() failed");
      return -1;
    }
    ret = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret < 0) {
      perror("ERROR: connect() failed");
      return -1;
    }
    printf("Data will be streamed to %s:%d.\n", dest_info->h_name, dest_port);
  }

  /* Start the Camera */
  printf("Starting capture ...\n");
  fflush(stdout);
  start_time = gettime();

  /* Main loop:
   */
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  int nSkipped = 0, nFrames = 0;

  PdvDev* dev = pDriver->dev();

  int width  = pdv_get_width (dev);
  int height = pdv_get_height(dev);
  int depth  = pdv_get_depth (dev);
  int size   = pdv_get_imagesize(dev);
  printf("image dimensions %d x %d x %d\n",width,height,depth);

  printf("Allocating buffers ...\n");
  fflush(stdout);
  pdv_multibuf(dev, 2);

  struct camstream_image_t sndimg;
  sndimg.base.hdr     = CAMSTREAM_IMAGE_HDR;
  sndimg.base.pktsize = CAMSTREAM_IMAGE_PKTSIZE;
  sndimg.format = (bitsperpixel>8) ? IMAGE_FORMAT_GRAY16 : IMAGE_FORMAT_GRAY8;
  sndimg.size   = htonl(size);
  sndimg.width  = htons(width);
  sndimg.height = htons(height);
  sndimg.data_off = htonl(4);	/* Delta between ptr address and data */

  for(i=1; i != nimages+1; i++) {
    int tosend;
    printf("\nStarting frame %d ... ",i);
    fflush(stdout);
    
    pdv_start_images(dev,1);

    printf("\nWaiting frame %d ... ",i);
    fflush(stdout);

    u_char* pFrame = pdv_wait_images(dev,1);

    printf("\nCapture done...");
    fflush(stdout);

    if (pdv_timeouts(dev) > 0) {
      // timedout
      printf("timedout ... continue\n");
      i--;
      continue;
    }

    if (!dest_host) {
       printf( "a\n" );
      if (pFrame == NULL) {
         printf( "b\n" );
	nSkipped++;
	if ((nSkipped%ifps)==0) {
	  printf("skipped %d frames\n",nSkipped);
	}
      }
      else {
	
	nFrames++;
	//	delete pFrame;
      }
      continue;
    }

    if (pFrame == NULL) {
      printf("skipped.\nGetFrameHandle returned NULL.\n");
      continue;
    }

    /*
    printf( "sndimg.base.hdr = %u.\n", sndimg.base.hdr );
    printf( "sndimg.base.pktsize = %u.\n", ntohl(sndimg.base.pktsize) );
    printf( "sndimg.format = %d.\n", sndimg.format );
    printf( "sndimg.size = %u.\n", sndimg.size );
    printf( "sndimg.width = %hu.\n", sndimg.width );
    printf( "sndimg.height = %hu.\n", sndimg.height );
    printf( "sndimg.data_off = %u.\n", sndimg.data_off );
    */
    ret = send(sockfd, (char *)&sndimg, sizeof(sndimg), 0);
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "send(hdr): %s.\n", strerror(errno));
      //      delete pFrame;
      delete pCamera;
      return -1;
    }
      
    {
      u_char* p;
      /*
      printf( "pFrame->Data: [0]=0x%08x, [1]=0x%08x, [2]=0x%08x, [3]=0x%08x\n",
              ((unsigned int*)pFrame->data)[0], 
              ((unsigned int*)pFrame->data)[1], 
              ((unsigned int*)pFrame->data)[2], 
              ((unsigned int*)pFrame->data)[3] );
      */
      for(tosend = size, p=pFrame;
	  tosend > 0; tosend -= ret, p += ret) {
	ret = send(sockfd, p, 
		   tosend < CAMSTREAM_PKT_MAXSIZE ? 
		   tosend : CAMSTREAM_PKT_MAXSIZE, 0);
	if (ret < 0) {
	  printf("failed.\n");
	  fprintf(stderr, "send(img): %s.\n", strerror(errno));
	  //	  delete pFrame;
	  close(sockfd);
	  delete pCamera;
	  return -1;
	} 
      }
      //      delete pFrame;
    }

  }
  printf("done.\n");
  close(sockfd);

  /* Stop the camera */
  printf("Stopping camera ... ");
  delete pCamera;
  end_time = gettime();
  printf("done.\n"); 

  printf("done.\n");
  printf("Transmitted %u images in %0.3fs => %0.2ffps\n\n", i-1,
	 (double)(end_time-start_time)/1000000,
	 ((double)(i-1)*1000000)/(end_time-start_time));
  return 0;
}
