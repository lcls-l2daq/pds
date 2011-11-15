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
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/camera/LvCamera.hh"
#include "camstream.h"
#include "pthread.h"

#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FccdCamera.hh"

#include <vector>
using std::vector;

#define USE_TCP

#define OPALGAIN_MIN      1.
#define OPALGAIN_MAX      32.
#define OPALGAIN_DEFAULT  30.

#define OPALEXP_MIN       0.01
#define OPALEXP_MAX       320.00

using namespace PdsLeutron;

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
  LvCamera::Status Status;
  char *dest_host = NULL;
  char *strport;
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

  vector<int> grabberid;
  vector<int> camsignal;

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
      char* endPtr = argv[++i];
      do { 
        grabberid.push_back(strtoul(endPtr,&endPtr,0));
      } while (*(endPtr++)==',');
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
  vector<PicPortCL*> pCamera;

  switch(camera_choice) {
  case 0:
    {
      Opal1kConfigType* Config = new Opal1kConfigType( 32, 100, 
						       bitsperpixel==8 ? Opal1kConfigType::Eight_bit : 
						       bitsperpixel==10 ? Opal1kConfigType::Ten_bit :
						       Opal1kConfigType::Twelve_bit,
						       Opal1kConfigType::x1,
						       Opal1kConfigType::None,
						       true, false);
      
      for(unsigned i=0; i<grabberid.size(); i++) {
        Opal1kCamera* oCamera = new Opal1kCamera( "OpalId0", grabberid[i] );
        oCamera->set_config_data(Config);
        pCamera.push_back(oCamera);
      }
      break;
    }
  case 2:
    {
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
      for(unsigned i=0; i<grabberid.size(); i++) {
        FccdCamera* fCamera = new FccdCamera( "FccdId0", grabberid[i] );
        fCamera->set_config_data(Config);
        pCamera.push_back(fCamera);
      }
      break;
    }
  case 1:
  default:
    {
      TM6740ConfigType* Config = new TM6740ConfigType( 0x28,  // black-level-a
						       0x28,  // black-level-b
						       0xde,  // gain-tap-a
						       0xe9,  // gain-tap-b
						       false, // gain-balance
						       TM6740ConfigType::Eight_bit,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::Linear );
      for(unsigned i=0; i<grabberid.size(); i++) {
        TM6740Camera* tCamera = new TM6740Camera( "PulnixId0", grabberid[i] );
        tCamera->set_config_data(Config);
        pCamera.push_back(tCamera);
      }
      break;
    }
  }

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

  /* Create a second thread. This thread is only here
   * as a catch all for signals, because LVSDS use a few
   * real-time signals. If we do not do that then this
   * thread system call keep being interrupted.
   */
  pthread_create(&th2, NULL, thread2, &camsigset_th2);
  /* block all signals in this thread, thread2 will take care of them */
  sigfillset(&camsigset);
  pthread_sigmask(SIG_BLOCK, &camsigset, 0);

  for(unsigned i=0; i<grabberid.size(); i++) {
    /* Configure the camera */
    printf("Configuring camera ... %d",i); 
    fflush(stdout);
  
    if ((camsig = pCamera[i]->SetNotification(LvCamera::NOTIFYTYPE_SIGNAL)) < 0) {
      printf("failed.\n");
      fprintf(stderr, "Camera::SetNotification: %s.\n", strerror(-ret));
      delete pCamera[i];
      return -1;
    }
    printf("done.\n");

    /* Initialize the camera */
    printf("Initialize camera ... "); 
    fflush(stdout);
    ret = pCamera[i]->Init();
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
      delete pCamera[i];
      return -1;
    }

    if( camera_choice == 0 ) {
      ((Opal1kCamera*) pCamera[i])->setTestPattern( test_pattern );
    }

    // Continuous Mode
    const int SZCOMMAND_MAXLEN = 32;
    char szCommand [SZCOMMAND_MAXLEN];
    char szResponse[SZCOMMAND_MAXLEN];

#define SetParameter(title,cmd,val1) { \
  unsigned val = val1; \
  printf("Setting %s = d%u\n",title,val); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val); \
  ret = pCamera[i]->SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  unsigned rval; \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = pCamera[i]->SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
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

    printf("Starting capture ...\n");
    fflush(stdout);
    start_time = gettime();
    ret = pCamera[i]->Start();
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
      close(sockfd);
      delete pCamera[i];
      return -1;
    }

    camsignal.push_back(camsig);
  }

  /* Main loop:
   * 1- wait for a signal
   * 2- get the next frame
   */

  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  for(i=1; i != nimages+1; i++) {
    int signal;
    struct camstream_image_t sndimg;
    int tosend;
    FrameHandle *pFrame;
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
      //      delete pCamera;
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
      //      delete pCamera;
      return -1;
    }   
    
    for(int j=0; j<camsignal.size(); j++) {
      if(signal != camsignal[j])
        continue;

      printf(" signal %d from camera %d ..", signal, j);

      pFrame = pCamera[j]->GetFrameHandle();

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

      ret = send(sockfd, (char *)&sndimg, sizeof(sndimg), 0);
      if (ret < 0) {
        printf("failed.\n");
        fprintf(stderr, "send(hdr): %s.\n", strerror(errno));
        delete pFrame;
        delete pCamera[j];
        return -1;
      }
      
      {
        char* p;
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
            delete pCamera[j];
            return -1;
          } 
        }
        delete pFrame;
      }
    }
    printf("\n");
  }

  printf("done.\n");
  close(sockfd);
  close(fdpipe[0]);
  close(fdpipe[1]);

  for(unsigned j=0; j<grabberid.size(); j++) {
    /* Stop the camera */
    printf("Stopping camera ... ");
    fflush(stdout);
    ret = pCamera[j]->Stop();
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "Camera::Stop: %s.\n", strerror(-ret));
      delete pCamera[j];
      return -1;
    }
    end_time = gettime();
    printf("done.\n"); 

    /* Get some statistics */
    printf("Checking camera status ... "); 
    fflush(stdout);
    ret = pCamera[j]->GetStatus(Status);
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "Camera::GetStatus: %s.\n", strerror(-ret));
      delete pCamera[j];
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
    delete pCamera[j];
  }
  return 0;
}
