#include <stdio.h>
#include <unistd.h>     /* for sleep() */
#include <pthread.h>

#include "rnxState.h"
#include "triggerThread.h"
#include "workThread.h"
#include "rnxlog.h"
#include "rnxcommon.h"

#include <sys/types.h>
#include <sys/socket.h>

#define RNX_TRIGGER_PORT  30051

int createUdpSocket(int port);

void *triggerRoutine(void *arg)
{
  trigger_state_t *myState = (trigger_state_t *)arg;
  int tid = myState->task_id;
  int mode = myState->mode;
  workCmd_t workCmd;
  int triggerFd;
  const char *modeString;
  int     recvlen;
  char    trigbuf[512];

  workCmd.cmd = RNX_WORK_FRAMEREADY;
  workCmd.epoch = 0;

  char msg[LOGBUF_SIZE];

  switch (mode) {
    case 0:
      modeString = "Timer";
      break;
    case 1:
      modeString = "UDP";
      break;
    default:
      modeString = "Unsupported";
      break;
  }
  sprintf(msg, "trigger thread #%d started in mode %d (%s)", tid, mode, modeString);
  INFO_LOG(msg);

  switch (mode) {
    case RNX_TRIGGER_MODE_TIMER:
      while (1) {
        if (rnxState() == RNX_STATE_ENABLED) {
          if (write(myState->write_pipe_fd, &workCmd, sizeof(workCmd)) == -1) {
            perror("write");
            INFO_LOG("write failed");
          }
        }
        sleep(1);
      }
      break;

    case RNX_TRIGGER_MODE_UDP:
      triggerFd = createUdpSocket(RNX_TRIGGER_PORT);
      if (triggerFd) {
        INFO_LOG("Trigger socket open");
      } else {
        sprintf(msg, "Error: createUdpSocket(%d) failed", RNX_TRIGGER_PORT);
        printf("%s\n", msg);
        ERROR_LOG(msg);
        break;
      }
      while (1) {
        /* wait for UDP packet */
        recvlen = recvfrom(triggerFd, trigbuf, sizeof(trigbuf), 0, 0, 0);
        /* write to work pipe */
        if (rnxState() == RNX_STATE_ENABLED) {
          if (write(myState->write_pipe_fd, &workCmd, sizeof(workCmd)) == -1) {
            perror("write");
            INFO_LOG("write failed");
          }
        }
      }
      break;

    default:
      sprintf(msg, "Error: %s: trigger mode %d not supported", __FUNCTION__, mode);
      printf("%s\n", msg);
      ERROR_LOG(msg);
      break;
  }
  sprintf(msg, "Bye from triggerThread #%d!", tid);
  printf("%s\n", msg);
  INFO_LOG(msg);
  pthread_exit(NULL);
}

#include <string.h>
#include <netinet/in.h>

int createUdpSocket(int port)
{
  struct sockaddr_in myaddr; /* our address */
  int fd; /* our socket */

  /* create a UDP socket */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return 0;
  }

  /* bind the socket to any valid IP address and a specific port */
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);
  if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
    perror("bind");
    return 0;
  }

  return (fd);
}
