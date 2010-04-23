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

#include "pds/zerocopy/dma_splice/dma_splice.h"

#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FccdCamera.hh"

#define USE_TCP

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
    "--splice: use splice.\n"
    "--fps <nframes>: frames per second.\n"
    "--shutter: use external shutter.\n"
    "--camera {0|1|2}: choose Opal (0) or Pulnix (1) or FCCD (2).\n"
    "--help: display this message.\n"
    "\n", progname, progname);
  return;
}

/* thread only here to catch signals */
void *thread2(void *arg)
{
  pthread_sigmask(SIG_BLOCK, (sigset_t *)arg, 0);
  while(1) sleep(100);
  return 0;
}

void *frame_cleanup(void *arg)
{
  unsigned fd = *(unsigned*)arg;
  unsigned long release_arg;
  int ret;
  do {
    if (!(ret=dma_splice_notify(fd, &release_arg)) ) {
      FrameHandle* pFrame = reinterpret_cast<FrameHandle*>(release_arg);
      printf("Deleting %p\n",pFrame);
      delete pFrame;
    }
    else
      printf("notify failed %s\n",strerror(-ret));
  } while(1);
  return 0;
}

int main(int argc, char *argv[])
{ // int exttrigger = 0;
  int extshutter = 0;
  double fps = 0;
  int ifps =0;
  int usplice = 0;
  int i, ret, sockfd=-1;
  long nimages = 0;
  unsigned long long start_time, end_time;
  int camera_choice = 0;
  LvCamera::Status Status;
  char *dest_host=NULL;
  char *strport;
  char* grabber;
  int dest_port = CAMSTREAM_DEFAULT_PORT;
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

  struct camstream_image_t xxx;
  unsigned long camstream_image_t_len = sizeof(struct camstream_image_t);
  
  printf( "camreceiver: sizeof(camstream_image_t) = %lu.\n",
          camstream_image_t_len );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t) = %lu.\n",
          sizeof(xxx.base) );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t.hdr) = %lu.\n",
          sizeof(xxx.base.hdr) );
  printf( "camreceiver: sizeof(camstream_img_t.camstream_basic_t.pktsize) = %lu.\n",
          sizeof(xxx.base.pktsize) );
  printf( "camreceiver: sizeof(camstream_img_t.format) = %lu.\n",
          sizeof(xxx.format) );
  printf( "camreceiver: sizeof(camstream_img_t.size) = %lu.\n",
          sizeof(xxx.size) );
  printf( "camreceiver: sizeof(camstream_img_t.width) = %lu.\n",
          sizeof(xxx.width) );
  printf( "camreceiver: sizeof(camstream_img_t.height) = %lu.\n",
          sizeof(xxx.height) );
  printf( "camreceiver: sizeof(camstream_img_t.data_off) = %lu.\n",
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
    } else if( strcmp("--grabber", argv[i]) == 0 ) {
       grabber = argv[i];
    } else if( strcmp("--testpat", argv[i]) == 0 ) {
       // Currently only valid for the Opal1K camera!
       test_pattern = true;
    } else if (strcmp("--splice",argv[i]) == 0) {
#if ( __GNUC__ > 3 )
      usplice = dma_splice_open();
      if (usplice < 0) {
	printf("dma_splice_open error %s\n",strerror(-usplice));
	return usplice;
      }
#else
      printf("splice only supported in GCC version 4+\n");
#endif
    } else if (strcmp("--fps",argv[i]) == 0) {
      fps = strtod(argv[++i],NULL);
      ifps = int(fps);
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
  PicPortCL* pCamera(0);

  switch(camera_choice) {
  case 0:
    {
      Opal1kCamera* oCamera = new Opal1kCamera( "OpalId0" );
      Opal1kConfigType* Config = new Opal1kConfigType( 32, 100, 
						       bitsperpixel==8 ? Opal1kConfigType::Eight_bit : 
						       bitsperpixel==10 ? Opal1kConfigType::Ten_bit :
						       Opal1kConfigType::Twelve_bit,
						       Opal1kConfigType::x1,
						       Opal1kConfigType::None,
						       true, false);
      
      oCamera->Config(*Config);
      pCamera = oCamera;
      break;
    }
  case 2:
    {
      FccdCamera* fCamera = new FccdCamera();
      FccdConfigType* Config = new FccdConfigType();

      fCamera->Config(*Config);
      pCamera = fCamera;
      break;
    }
  case 1:
  default:
    {
      TM6740Camera* tCamera = new TM6740Camera();
      TM6740ConfigType* Config = new TM6740ConfigType( 0x28,  // black-level
						       0xde,  // gain-tap-a
						       0xe9,  // gain-tap-b
						       unsigned(1.e6/fps),  // shutter-width-us
						       false, // gain-balance
						       TM6740ConfigType::Eight_bit,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::x1,
						       TM6740ConfigType::Linear );
      tCamera->Config(*Config);
      pCamera = tCamera;
      break;
    }
  }

  /* Configure the camera */
  printf("Configuring camera ... "); 
  fflush(stdout);
  
  if ((camsig = pCamera->SetNotification(LvCamera::NOTIFYTYPE_SIGNAL)) < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::SetNotification: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }
  printf("done.\n");

  /* Initialize the camera */
  printf("Initialize camera ... "); 
  fflush(stdout);
  ret = pCamera->Init();
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }

  if( camera_choice == 0 ) {
     ((Opal1kCamera*) pCamera)->setTestPattern( test_pattern );
  }

  // Continuous Mode
  const int SZCOMMAND_MAXLEN = 32;
  char szCommand [SZCOMMAND_MAXLEN];
  char szResponse[SZCOMMAND_MAXLEN];

#define SetParameter(title,cmd,val1) { \
  unsigned val = val1; \
  printf("Setting %s = d%u\n",title,val); \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val); \
  ret = pCamera->SendCommand(szCommand, NULL, 0); \
  if (ret<0) return ret; \
  unsigned rval; \
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd); \
  ret = pCamera->SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN); \
  sscanf(szResponse+2,"%u",&rval); \
  printf("Read %s = d%u\n", title, rval); \
  if (rval != val) return -EINVAL; \
}

  if (!extshutter && camera_choice==0) {
    //    unsigned _mode=0, _fps=100000/fps, _it=540/10;
    unsigned _mode=0, _fps=unsigned(100000/fps), _it=_fps-10;
    unsigned _gain=3000;
    SetParameter ("Operating Mode","MO",_mode);
    SetParameter ("Frame Period","FP",_fps);
    // SetParameter ("Frame Period","FP",813);
    SetParameter ("Integration Time","IT", _it);
    // SetParameter ("Integration Time","IT", 810);
    SetParameter ("Digital Gain","GA", _gain);
  }

  printf("done.\n"); 

  if (usplice > 0)
    dma_splice_initialize( usplice, pCamera->frameBufferBaseAddress(),
			   pCamera->frameBufferEndAddress() - pCamera->frameBufferBaseAddress());

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

  if (usplice > 0) {
    pthread_t th3;
    pthread_create(&th3, NULL, frame_cleanup, &usplice);
  }

  printf("Starting capture ...\n");
  fflush(stdout);
  start_time = gettime();
  ret = pCamera->Start();
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
  char* spliceBuffer = new char[64*1024];

  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  int nSkipped = 0, nFrames = 0;

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

    pFrame = pCamera->GetFrameHandle();
    printf( "\npFrame = 0x%p.\n", pFrame );
    printf( "dest_host = 0x%p.\n", dest_host );

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
	
	unsigned thisId;
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
        printf("frameId= %x\n",thisId);
        printf("frame @ %p (%p)\n",pFrame->data, pFrame);

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

    printf("frame @ %p (%p)\n",pFrame->data, pFrame);
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

    if (usplice > 0)
      dma_splice_queue(usplice, 
		       pFrame->data, 
		       pFrame->width*pFrame->height*pFrame->elsize, 
		       (unsigned long)pFrame);

    printf( "sndimg.base.hdr = %lu.\n", sndimg.base.hdr );
    printf( "sndimg.base.pktsize = %lu.\n", ntohl(sndimg.base.pktsize) );
    printf( "sndimg.format = %d.\n", sndimg.format );
    printf( "sndimg.size = %lu.\n", sndimg.size );
    printf( "sndimg.width = %hu.\n", sndimg.width );
    printf( "sndimg.height = %hu.\n", sndimg.height );
    printf( "sndimg.data_off = %lu.\n", sndimg.data_off );
    ret = send(sockfd, (char *)&sndimg, sizeof(sndimg), 0);
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "send(hdr): %s.\n", strerror(errno));
      delete pFrame;
      delete pCamera;
      return -1;
    }
      
#if ( __GNUC__ > 3 )
    if (usplice > 0) {
      // Try to splice
      for(tosend = pFrame->width*pFrame->height*pFrame->elsize;
	  tosend > 0; tosend -= ret) {
	ret = ::splice(usplice, NULL, pipeFd[1], NULL, tosend, SPLICE_F_MOVE);
	if (ret < 0) {
	  printf("splice failed %s\n",strerror(-ret));
	  //	  delete pFrame;
	  close(sockfd);
	  close(usplice);
	  delete pCamera;
	  return -1;
	}
	
	int nbytes = ::read(pipeFd[0], spliceBuffer, ret);
	if (nbytes != ret) {
	  printf("read failed to copy spliced bytes %d/%d\n",nbytes,ret);
	}

	ret = send(sockfd, spliceBuffer, nbytes, 0);
	if (ret < 0) {
	  printf("send failed %s\n",strerror(-ret));
	  //	  delete pFrame;
	  close(sockfd);
	  close(usplice);
	  delete pCamera;
	  return -1;
	}
      }
    }
    else {
#else
    {
#endif
      char* p;
      printf( "pFrame->Data: [0]=0x%08x, [1]=0x%08x, [2]=0x%08x, [3]=0x%08x\n",
              ((unsigned int*)pFrame->data)[0], 
              ((unsigned int*)pFrame->data)[1], 
              ((unsigned int*)pFrame->data)[2], 
              ((unsigned int*)pFrame->data)[3] );
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
  if (usplice > 0)  close(usplice);

  /* Stop the camera */
  printf("Stopping camera ... ");
  fflush(stdout);
  ret = pCamera->Stop();
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
  ret = pCamera->GetStatus(Status);
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::GetStatus: %s.\n", strerror(-ret));
    delete pCamera;
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
  delete pCamera;
  return 0;
}
