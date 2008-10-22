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
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "kmemory.h"
#include "Camera.hh"
#include "camstream.h"
#include "pthread.h"

#include "pds/camera/Opal1000.hh"
#define CAMERA_CLASS  Pds::Opal1000
#define str2(a)       #a
#define str(a)        str2(a)
#define CAMERA_NAME   str(CAMERA_CLASS)
#define MAX_PACKET_SIZE CAMSTREAM_PKT_MAXSIZE

static unsigned long long gettime(void)
{	struct timeval t;

	gettimeofday(&t, NULL);
	return t.tv_sec*1000000+t.tv_usec;
}

static void help(const char *progname)
{
  printf("%s: stream data from a Camera object to the network.\n"
    "Syntax: %s [ -f ] <dest> [ --count <nimages> ] \\\n"
    "\t--trigger | --shutter | --fps <fps> | --help\n"
    "<dest>: if -f is specified dest is a file, otherwise\n"
    "\tit is the network address of the host to send the\n"
    "\tdata to (format <host>[:<port>]).\n"
    "--trigger: set to have the camera use an external trigger.\n"
    "--shutter: set the camera to use an external shutter control.\n"
    "--count <nimages>: number of images to acquire.\n"
    "--fps <fps>: number of frames per second.\n"
    "--help: display this message.\n"
    "\n", progname, progname);
  return;
}

/* Variables used accross threads */
char *dest_host = NULL;
char *dest_file = NULL;
int fdIPCPipe[2];
int ret_thread_streaming;
int fdKmemory;

/* thread only here to catch signals */
void *thread_signals(void *arg)
{
  pthread_sigmask(SIG_BLOCK, (sigset_t *)arg, 0);
  while(1) sleep(100);
}

/* This thread sends the data on the network using the zero copy stack */
void *thread_streaming(void *arg)
{ struct iovec iov;
  fd_set notify_set;
  int fdStreamPipe[2];
  char *strport;
  int dest_port = CAMSTREAM_DEFAULT_PORT;
  struct sockaddr_in dest_addr;
  struct hostent *dest_info;
  int ret, sockfd;
  unsigned long count;

  /* Set the signal mask */
  pthread_sigmask(SIG_BLOCK, (sigset_t *)arg, 0);

  /* First open the pipe */
  if (pipe(fdStreamPipe) < 0) {
    perror("pipe() failed");
    ret_thread_streaming = errno;
    goto err_pipe;
  }

  /* Do we stream to a socket or a file ? */
  if (dest_host != NULL) {
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
      ret_thread_streaming = EINVAL;
      goto err_socket;
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
      ret_thread_streaming = errno;
      goto err_socket;
    }
    ret = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret < 0) {
      perror("ERROR: connect() failed");
      ret_thread_streaming = errno;
      goto err_beforeloop;
    }
    printf("Data will be streamed to %s:%d.\n", dest_info->h_name, dest_port);
  } else if (dest_file != NULL) {
    sockfd = open(dest_file, O_WRONLY | O_CREAT);
    if (sockfd < 0) {
      fprintf(stderr, "open() failed for file %s: %s", dest_file, strerror(errno));
      ret_thread_streaming = errno;
      goto err_socket;
    }
  }
  
  /* main loop */
  while(1) {
    int sent;

    FD_ZERO(&notify_set);
    FD_SET(fdIPCPipe[0], &notify_set);
    ret = select(fdIPCPipe[0]+1, &notify_set, NULL, NULL, NULL);
    if (ret < 0) {
      perror("select()");
      ret_thread_streaming = errno;
      goto err_loop;
    } else if ((ret <= 0) || (!FD_ISSET(fdIPCPipe[0], &notify_set))) {
      continue;
    }
    ret = read(fdIPCPipe[0], &iov, sizeof(iov));
    if (ret == 0) {
      /* EOF */
      break;
    } else if (ret != sizeof(iov)) {
      fprintf(stderr, "read() returned %d: %s.\n", ret, strerror(errno));
      continue;
    }
    ret = kmemory_unmap(fdKmemory, &iov, 1);
    if (ret < 0) {
      fprintf(stderr, "kmemory_unmap() returned an error: %s.\n", strerror(-ret));
      continue;
    }
    for (count = 0; count < iov.iov_len; count += sent)
    { int tosend;
      tosend = iov.iov_len-count < MAX_PACKET_SIZE ? iov.iov_len-count : MAX_PACKET_SIZE;
      tosend = splice(fdKmemory, NULL, fdStreamPipe[1], NULL, tosend, SPLICE_F_MOVE);
      if (tosend < 0) {
        perror("splice(kmem->pipe)");
        sent = 0;
        continue;
      }
      for (sent = 0; sent < tosend; sent += ret) {
        ret = splice(fdStreamPipe[0], NULL, sockfd, NULL, tosend-sent, SPLICE_F_MOVE);
        if (ret < 0) {
          perror("splice(pipe->socket)");
          ret = 0;
          continue;
        }
      }
    }
  }
  ret_thread_streaming = 0;

err_loop:
err_beforeloop:
  close(sockfd);
err_socket:
  close(fdStreamPipe[0]);
  close(fdStreamPipe[1]);
err_pipe:
  close(fdIPCPipe[0]);
  return (void *)ret_thread_streaming;
}

int main(int argc, char *argv[])
{ int exttrigger = 0;
  int extshutter = 0;
  int fps = 0;
  int i, ret;
  long nimages = 0;
  unsigned long long start_time, end_time;
  CAMERA_CLASS *pCamera;
  Pds::Camera::Config Config;
  Pds::Camera::Status Status;
  int camsig;
  int thret;
  sigset_t sigset_thread_signals, sigset_thread_capture, sigset_thread_streaming;
  pthread_t h_thread_signals, h_thread_streaming;

  /* Parse the command line */
  for (i = 1; i < argc; i++) {
    if (strcmp("--help",argv[i]) == 0) {
      help(argv[0]);
      return 0;
    } else if (strcmp("--trigger",argv[i]) == 0) {
      exttrigger = 1;
    } else if (strcmp("--shutter",argv[i]) == 0) {
      extshutter = 1;
    } else if (strcmp("--fps",argv[i]) == 0) {
      fps = atol(argv[++i]);
    } else if (strcmp("--count",argv[i]) == 0) {
      nimages = atol(argv[++i]);
    } else if (strcmp("-f",argv[i]) == 0) {
      dest_file = argv[++i];
    } else if (dest_host == NULL) {
      dest_host = argv[i];
    } else {
      fprintf(stderr, "ERROR: Invalid argument %s, try --help.\n", 
          argv[i]);
      return -1;
    }
  }
  if((fps == 0) && (!exttrigger) && (!extshutter)) {
    fprintf(stderr, "ERROR: you must set the frames per second with --fps.\n");
    return -1;
  }
  if (dest_file != NULL) {
    dest_host = NULL;
  }

  /* Open the camera */
  printf("Creating camera object of class %s ... ",CAMERA_NAME);
  fflush(stdout);
  pCamera = new CAMERA_CLASS();
  printf("done.\n");

  /* Configure the camera */
  printf("Configuring camera ... "); 
  fflush(stdout);
  if (exttrigger)
    Config.Mode = Pds::Camera::MODE_EXTTRIGGER;
  else if (extshutter)
    Config.Mode = Pds::Camera::MODE_EXTTRIGGER_SHUTTER;
  else
    Config.Mode = Pds::Camera::MODE_CONTINUOUS;
  Config.Format = Pds::Frame::FORMAT_GRAYSCALE_8;
  Config.GainPercent = 10;
  Config.BlackLevelPercent = 10;
  Config.ShutterMicroSec = 1000000/fps;
  Config.FramesPerSec = fps;
  ret = pCamera->SetConfig(Config);
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::SetConfig: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }
  camsig = pCamera->SetNotification(Pds::Camera::NOTIFYTYPE_SIGNAL);
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::SetNotification: %s.\n", strerror(-ret));
    delete pCamera;
    return -1;
  }
  printf("done.\n");

  /* First open the pipe */
  printf("Initialize IPC pipe and kmemory ... "); 
  if (pipe(fdIPCPipe) < 0) {
    printf("failed.\n");
    perror("pipe() failed");
    delete pCamera;
    return -1;
  }
  fdKmemory = kmemory_open();
  if (fdKmemory < 0) {
    printf("failed.\n");
    fprintf(stderr, "kmemory_open failed: %s.\n", strerror(-fdKmemory));
    delete pCamera;
    return -1;
  }
  printf("done.\n"); 

  printf("Create threads and setup signals ... "); 
  /* Create a second thread. This thread is only here
   * as a catch all for signals, because LVSDS use a few
   * real-time signals. If we do not do that then this
   * thread system call keep being interrupted.
   */
  sigemptyset(&sigset_thread_signals);
  sigaddset(&sigset_thread_signals, camsig);
  pthread_create(&h_thread_signals, NULL, thread_signals, &sigset_thread_signals);

  /* block all signals in the streaming thread */
  sigfillset(&sigset_thread_streaming);
  pthread_sigmask(SIG_BLOCK, &sigset_thread_streaming, 0);
  pthread_create(&h_thread_streaming, NULL, thread_streaming, &sigset_thread_streaming);

  /* block all signals in this thread, thread_signals will take care of them */
  sigfillset(&sigset_thread_capture);
  pthread_sigmask(SIG_BLOCK, &sigset_thread_capture, 0);

  /* setup variables for sigwait */
  sigemptyset(&sigset_thread_capture);
  sigaddset(&sigset_thread_capture, camsig);
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
  printf("done.\n"); 
  
  /* Start the Camera */
  printf("Starting capture ...\n");
  fflush(stdout);
  start_time = gettime();
  ret = pCamera->Start();
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Init: %s.\n", strerror(-ret));
    delete pCamera;
    exit(-1);
  }

  /* Main loop:
   * 1- wait for a signal
   * 2- get the next frame
   * 3- send the frame to the streaming thread
   */
  for(i=1; i != nimages+1; i++) {
    int signal;
    Pds::Frame *pFrame;
    struct camstream_image_t *psndimg;
    unsigned long image_size;
    struct iovec iov;

    printf("\rCapturing frame %d ... ",i);
    /* Wait for a new frame */
    ret = sigwait(&sigset_thread_capture, &signal);
    if (ret != 0) {
      printf("failed.\n");
      fprintf(stderr, "sigwait: %s.\n", strerror(ret));
      delete pCamera;
      exit(-1);
    }
    if(signal != camsig) {
      printf("???\nWarning: got signal %d, expected %d.\n", signal, camsig);
      i--;
      continue;
    }
    /* Retrieve the last captured frame */
    pFrame = pCamera->GetFrame();
    if (pFrame == NULL) {
      printf("skipped.\nGetFrame returned NULL.\n");
      continue;
    }
    /* Allocate a buffer and copy the image structure and data */
    image_size = pFrame->width*pFrame->height*pFrame->elsize;
    ret = kmemory_map_write(fdKmemory, &iov, 1, sizeof(struct camstream_image_t) + image_size);
    if (ret < 0) {
      printf("failed.\n");
      fprintf(stderr, "kmemory_map_write failed: %s.\n",strerror(-ret));
      delete pCamera;
      exit(-1);
    }
    psndimg = (struct camstream_image_t *)iov.iov_base;
    psndimg->base.hdr = CAMSTREAM_IMAGE_HDR;
    psndimg->base.pktsize = CAMSTREAM_IMAGE_PKTSIZE;
    psndimg->format = IMAGE_FORMAT_GRAY8;
    psndimg->size = htonl(pFrame->width*pFrame->height*pFrame->elsize);
    psndimg->width = htons(pFrame->width);
    psndimg->height = htons(pFrame->height);
    psndimg->data = (void *)htonl(4);	/* Delta between ptr address and data */
    memcpy((char *)iov.iov_base + sizeof(struct camstream_image_t), pFrame->data, image_size);
    delete pFrame;
    /* Notify event builder a frame is ready */
    write(fdIPCPipe[1], &iov, sizeof(struct iovec));
  }
  printf("done.\n");
  close(fdIPCPipe[1]);

  /* Stop the camera */
  printf("Stopping camera ... ");
  fflush(stdout);
  ret = pCamera->Stop();
  if (ret < 0) {
    printf("failed.\n");
    fprintf(stderr, "Camera::Stop: %s.\n", strerror(-ret));
    delete pCamera;
    exit(-1);
  }
  end_time = gettime();
  printf("done.\n"); 
  printf("Stopping streaming thread ... ");
  pthread_cancel(h_thread_streaming);
  ret = pthread_join(h_thread_streaming, (void **)&thret);
  if (ret != 0) {
    printf("failed.\n");
    fprintf(stderr, "pthread_join: %s.\n", strerror(ret));
    delete pCamera;
    return -1;
  }
  if ((thret != 0) && (thret != -1)) {
    printf("returned %d.\n", thret);
    fprintf(stderr, "Streaming thread exited with error: %s.\n", strerror(thret));
  } else {
    printf("done.\n");
  }
  /* Now can safely close the kmemory handle */
  kmemory_close(fdKmemory);

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
