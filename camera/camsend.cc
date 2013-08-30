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
#include <signal.h>
#include <new>
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "camstream.h"
#include "pthread.h"

#include "pds/camera/PicPortCL.hh"
#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FccdCamera.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

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

// If we use select we have a signal handler that
// writes into a pipe every time a signal comes

int fdpipe[2];

static void pipe_notify(int signo)
{
  write(fdpipe[1], &signo, sizeof(signo));
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

/* thread only here to catch signals */
void *thread2(void *arg)
{
  pthread_sigmask(SIG_BLOCK, (sigset_t *)arg, 0);
  while(1) sleep(100);
  return 0;
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
  PdsLeutron::PicPortCL::Status Status;
  char *dest_host = NULL;
  char *strport;
  int  grabberid  = 0;
  int dest_port   = CAMSTREAM_DEFAULT_PORT;
  struct sockaddr_in dest_addr;
  struct hostent *dest_info;
  int camsig;
  sigset_t camsigset;
  sigset_t camsigset_th2;
  pthread_t th2;
  struct sigaction sa_notification;
  fd_set notify_set;
  int bitsperpixel = 8;
  bool test_pattern;

  int pipeFd[2];
  if ((ret=::pipe(pipeFd)) < 0) {
    printf("pipe open error %s\n",strerror(-ret));
    return ret;
  }

  /*
  struct camstream_image_t xxx;
  unsigned long camstream_image_t_len = sizeof(struct camstream_image_t);
  
  printf( "camreceiver: sizeof(camstream_image_t) = %lu.\n",
          camstream_image_t_len );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t) = %u.\n",
          sizeof(xxx.base) );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t.hdr) = %u.\n",
          sizeof(xxx.base.hdr) );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t.pktsize) = %u.\n",
          sizeof(xxx.base.pktsize) );
  printf( "camreceiver: sizeof(camstream_img_t.format) = %u.\n",
          sizeof(xxx.format) );
  printf( "camreceiver: sizeof(camstream_img_t.size) = %u.\n",
          sizeof(xxx.size) );
  printf( "camreceiver: sizeof(camstream_img_t.width) = %u.\n",
          sizeof(xxx.width) );
  printf( "camreceiver: sizeof(camstream_img_t.height) = %u.\n",
          sizeof(xxx.height) );
  printf( "camreceiver: sizeof(camstream_img_t.data_off) = %u.\n",
          sizeof(xxx.data_off) );

  xxx.base.hdr = 0x12345678;
  xxx.base.pktsize = 0x87654321;
  xxx.format = 0xA5;
  xxx.size = 0x87654321;
  xxx.width = 0x1234;
  xxx.height = 0x4321;
  xxx.data_off = 0x13243546;

  unsigned char* xxxptr = (unsigned char*) &xxx;
  unsigned row, col;
  for( row = 0; row < (camstream_image_t_len / 16 + 1); ++row )
  {
     printf( "%04d: ", row * 16 );
     for( col = 0; col < 16; ++col )
     {
        printf( " %02x", xxxptr[row*16 + col] );
     }
     printf( "\n" );
  }
  */

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

  printf( "Preparing camera...\n" );

  /* Open the camera */
  Pds::CameraBase* pCamera;

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
						       true, false, false, 0, 0, 0);
      
      oCamera->set_config_data(Config);
      pCamera = oCamera;
      break;
    }
  case 2:
    {
      FccdCamera* fCamera = new FccdCamera();
      const float dacVoltage[] = {  6.0,    // DAC 1
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
      };
      const uint16_t waveform[] = { 0xfe3f, // waveform0
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
      };
      FccdConfigType* Config = new FccdConfigType
                                    (
                                    0,      // outputMode: 0 == FIFO
                                    noccd ? false : true,   // CCD Enable
                                    ifps ? true : false,    // Focus Mode Enable
                                    1,      // internal exposure time (ms)
                                    dacVoltage,
                                    waveform );
      fCamera->set_config_data(Config);
      pCamera = fCamera;
      break;
    }
  case 1:
  default:
    {
      TM6740Camera* tCamera = new TM6740Camera();
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

  PdsLeutron::PicPortCL* pDriver = new PdsLeutron::PicPortCL(*pCamera,grabberid);

  /* Configure the camera */
  printf("Configuring camera ... "); 
  fflush(stdout);
  
  if ((camsig = pDriver->SetNotification(PdsLeutron::PicPortCL::NOTIFYTYPE_SIGNAL)) < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::SetNotification: %s.\n", strerror(-ret));
    delete pDriver;
    return -1;
  }
  printf("done.\n");

  /* Initialize the camera */
  printf("Initialize camera ... "); 
  fflush(stdout);
  
  GenericPool* msg_pool = new GenericPool(1,sizeof(UserMessage));
  UserMessage* msg = new (msg_pool)UserMessage;
  ret = pDriver->Init(msg);
  fprintf(stderr, "%s\n", msg->msg());

  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
    delete pDriver;
    return -1;
  }

  if( camera_choice == 0 ) {
    ((Opal1kCamera*) pCamera)->setTestPattern( *pDriver, test_pattern );
  }

  // Continuous Mode
  const int SZCOMMAND_MAXLEN = 32;
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];

#define SetParameter(title,cmd,val1) { \
  unsigned val = val1; \
  printf("Setting %s = d%u\n",title,val); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val); \
  ret = pDriver->SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  unsigned rval; \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = pDriver->SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  sscanf(szResponse+2,"%u",&rval); \
  printf("Read %s = d%u\n", title, rval); \
  if (rval != val) return -EINVAL; \
}

  if (!extshutter && camera_choice==0) {
    //    unsigned _mode=0, _fps=100000/fps, _it=540/10;
    unsigned _mode=0, _fps=unsigned(100000/fps), _it=_fps-10;
    SetParameter ("Operating Mode","MO",_mode);
    SetParameter ("Frame Period","FP",_fps);
    // SetParameter ("Frame Period","FP",813);
    if (opalexp > 0) {
      // exposure (integration time) has been set on cmd line
      _it = (unsigned) int(opalexp * 100.);
    }
    SetParameter ("Integration Time","IT", _it);
    // SetParameter ("Integration Time","IT", 810);
    SetParameter ("Digital Gain","GA", uopalgain100);
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
  {
    /* First open the pipe */
    if (pipe(fdpipe) < 0) {
      perror("pipe() failed");
      return -1;
    }
    /* Register the signal handler */
    sa_notification.sa_handler = pipe_notify;
    sigemptyset (&sa_notification.sa_mask);
    sa_notification.sa_flags = 0;
    sigaction (camsig, &sa_notification, NULL);
  }
  sigemptyset(&camsigset_th2);

  /* Create a second thread. This thread is only here
   * as a catch all for signals, because LVSDS use a few
   * real-time signals. If we do not do that then this
   * thread system call keep being interrupted.
   */
  pthread_create(&th2, NULL, thread2, &camsigset_th2);
  /* block all signals in this thread, thread2 will take care of them */
  sigfillset(&camsigset);
  pthread_sigmask(SIG_BLOCK, &camsigset, 0);

  printf("Starting capture ...\n");
  fflush(stdout);
  start_time = gettime();
  ret = pDriver->Start();
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
    close(sockfd);
    delete pCamera;
    return -1;
  }

  /* Main loop:
   * 1- wait for a signal
   * 2- get the next frame
   */
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  int nSkipped = 0, nFrames = 0;

  for(i=1; i != nimages+1; i++) {
    int signal;
    struct camstream_image_t sndimg;
    int tosend;
    PdsLeutron::FrameHandle *pFrame;
    printf("\rCapturing frame %d ... ",i);
    fflush(stdout);
    FD_ZERO(&notify_set);
    FD_SET(fdpipe[0], &notify_set);
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 2;

    ret = select(fdpipe[0]+1, &notify_set, NULL, NULL, &tv);

    if (ret < 0) {
      printf("failed.\n");
      perror("select()");
      close(sockfd);
      delete pCamera;
      return -1;
    } else if ((ret <= 0) || (!FD_ISSET(fdpipe[0], &notify_set))) {
      // timedout
      printf("timedout ... continue\n");
      i--;
      continue;
    }

    ret = read(fdpipe[0], &signal, sizeof(signal));
    if (ret != sizeof(signal)) {
      printf("failed.\n");
      fprintf(stderr, "read() returned %d: %s.\n", ret, strerror(errno));
      close(sockfd);
      delete pCamera;
      return -1;
    }   
    if(signal != camsig) {
      printf("???\nWarning: got signal %d, expected %d.\n", signal, camsig);
      i--;
      continue;
    }

    pFrame = pDriver->GetFrameHandle();

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
	
	unsigned thisId = 0;
	switch (bitsperpixel) {
	case 0:
	case 8:
	  { unsigned char* data = (unsigned char*)pFrame->data; 
	    thisId = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
	    break; }
	case 10:
	case 12:
	  { unsigned short* data = (unsigned short*)pFrame->data; 
	    thisId = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
	    break; }
	}

// 	if (thisId != ++frameId) {
// 	  printf("unexpected frameId %x -> %x\n",frameId,thisId);
// 	  frameId = thisId;
// 	}

	nFrames++;
	// if ((nFrames%ifps)==0) {
	//   timespec _tp;
	//   clock_gettime(CLOCK_REALTIME, &_tp);
	//   printf("%d/%d/%p : %g sec\n",
	// 	 nFrames,nSkipped,
	// 	 pFrame->data,
	// 	 (_tp.tv_sec-tp.tv_sec)+1.e-9*(_tp.tv_nsec-tp.tv_nsec));
	//   tp = _tp;
	// }
	delete pFrame;
      }
      continue;
    }

    if (pFrame == NULL) {
      printf("skipped.\nGetFrameHandle returned NULL.\n");
      continue;
    }

    sndimg.base.hdr = CAMSTREAM_IMAGE_HDR;
    sndimg.base.pktsize = CAMSTREAM_IMAGE_PKTSIZE;
    sndimg.format = (bitsperpixel>8) ? IMAGE_FORMAT_GRAY16 : IMAGE_FORMAT_GRAY8;
    sndimg.size = htonl(pFrame->width*pFrame->height*pFrame->elsize);
    sndimg.width = htons(pFrame->width);
    sndimg.height = htons(pFrame->height);
    sndimg.data_off = htonl(4);	/* Delta between ptr address and data */

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
      delete pFrame;
      delete pCamera;
      return -1;
    }
      
    {
      char* p;
      /*
      printf( "pFrame->Data: [0]=0x%08x, [1]=0x%08x, [2]=0x%08x, [3]=0x%08x\n",
              ((unsigned int*)pFrame->data)[0], 
              ((unsigned int*)pFrame->data)[1], 
              ((unsigned int*)pFrame->data)[2], 
              ((unsigned int*)pFrame->data)[3] );
      */
      for(tosend = pFrame->width*pFrame->height*pFrame->elsize, 
	    p=(char *)pFrame->data; 
	  tosend > 0; tosend -= ret, p += ret) {
	ret = send(sockfd, p, 
		   tosend < CAMSTREAM_PKT_MAXSIZE ? 
		   tosend : CAMSTREAM_PKT_MAXSIZE, 0);
	if (ret < 0) {
	  printf("failed.\n");
	  fprintf(stderr, "send(img): %s.\n", strerror(errno));
	  delete pFrame;
	  close(sockfd);
	  delete pCamera;
	  return -1;
	} 
      }
      delete pFrame;
    }

  }
  printf("done.\n");
  close(sockfd);
  close(fdpipe[0]);
  close(fdpipe[1]);

  /* Stop the camera */
  printf("Stopping camera ... ");
  fflush(stdout);
  ret = pDriver->Stop();
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Stop: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }
  end_time = gettime();
  printf("done.\n"); 

  /* Get some statistics */
  printf("Checking camera status ... "); 
  fflush(stdout);
  ret = pDriver->GetStatus(Status);
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::GetStatus: %s.\n", strerror(-ret));
    delete pDriver;
    return -1;
  }
  printf("done.\n");
  printf("Transmitted %d frames.\n",i-1);
  printf("Camera %s (id=%s) captured %lu and dropped %lu.\n",
          Status.CameraName, Status.CameraId,
          Status.CapturedFrames, Status.DroppedFrames);
		/* We are now officially done */
  printf("Captured %lu images in %0.3fs => %0.2ffps\n\n", Status.CapturedFrames,
            (double)(end_time-start_time)/1000000,
            ((double)Status.CapturedFrames*1000000)/(end_time-start_time));
  delete pDriver;
  return 0;
}
