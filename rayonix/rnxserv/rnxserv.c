
#define RNX_CMD_NOOP        0x6e6f6f70
#define RNX_CMD_HELLO       0x68656c6c
#define RNX_CMD_CONFIG      0x636f6e66
#define RNX_CMD_DARK        0x6461726b
#define RNX_CMD_ENABLE      0x656e6162
#define RNX_CMD_DISABLE     0x64697361
#define RNX_CMD_QUIT        0x71756974
#define RNX_CMD_HELP        0x68656c70
#define RNX_CMD_LOGS        0x6c6f6773
#define RNX_CMD_STAT        0x73746174
#define RNX_CMD_RESET       0x72657365
#define RNX_CMD_QUIT        0x71756974
#define RNX_CMD_BULB        0x62756c62

#define SUFFIX "\r\n"

#define RNX_REPLY_GREETING  "220 rnx ready" SUFFIX
#define RNX_REPLY_FAILED    "554 Failed" SUFFIX
#define RNX_REPLY_OK        "250 OK" SUFFIX
#define RNX_REPLY_HELO      "250 rnx v1" SUFFIX
#define RNX_REPLY_QUIT      "221 rnx closing transmission channel" SUFFIX
#define RNX_REPLY_BULB      "221 PulseBulb()" SUFFIX
#define RNX_REPLY_NORECOG   "500 Syntax error, command unrecognized" SUFFIX
#define RNX_REPLY_BADSEQ    "503 Bad sequence of commands" SUFFIX
#define RNX_REPLY_LOG3      "253 rnx logging level 3 (Error)" SUFFIX
#define RNX_REPLY_LOG4      "254 rnx logging level 4 (Warning)" SUFFIX
#define RNX_REPLY_LOG5      "255 rnx logging level 5 (Notice)" SUFFIX
#define RNX_REPLY_LOG6      "256 rnx logging level 6 (Info)" SUFFIX
#define RNX_REPLY_LOG7      "257 rnx logging level 7 (Debug)" SUFFIX
#define RNX_REPLY_LOGINV    "502 rnx logging level invalid" SUFFIX
#define RNX_REPLY_STATE1    "251 framesize=0 # Unconfigured" SUFFIX
#define RNX_REPLY_STATE2    "252 framesize=%d;epoch=%d;deviceID=%s;readoutMode=%d;testPattern=%d # Configured" SUFFIX
#define RNX_REPLY_STATE3    "253 framesize=%d;epoch=%d;deviceID=%s;readoutMode=%d;testPattern=%d # Enabled" SUFFIX
#define RNX_REPLY_STATEINV  "502 rnx state invalid" SUFFIX
#define RNX_REPLY_NOTIMPL   "502 Command not implemented" SUFFIX

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <sys/select.h>
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "helper.h"           /*  our own helper functions  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "workThread.h"
#include "triggerThread.h"
#include "rnxState.h"
#include "rnxlog.h"
#include "rnxcommon.h"

/*  Global constants  */

#define MAX_LINE            (1000)
#define MAX_DEVICEID        (40)

static int _rnxState = RNX_STATE_UNCONFIGURED;
static unsigned int _rnxEpoch = 0u;
static int _framesize = 0u;
static int _readoutMode = 0u;
static int _testPattern = 0u;
char       _deviceID[MAX_DEVICEID+1];
static int _rnxLogLevel = 7;
int _workPipeFd[2];
int _controlPipeFd[2];
static sem_t configSem;       /* semaphore protects config operations */
static unsigned _verbosity = 0;
static bool _simFlag = false;

void rnxStateReply(char *buf);
void rnxLogReply(char *buf);
void rnxReset(bool);
int translate(char *buf);
int create_threads(bool);
int frameCountInit();
int frameCountGet();
int configSemInit();

void usage(const char *name)
{
  printf("Usage: %s [OPTIONS]\n\n", name);
  printf("OPTIONS:\n");
  printf("  -s         Use simulator\n");
  printf("  -v         Be verbose (may be repeated)\n");
}

int main(int argc, char *argv[]) {
  int       list_s;                /*  listening socket          */
  int       conn_s;                /*  connection socket         */
  short int port = RNX_CONTROL_PORT;
  struct    sockaddr_in servaddr;  /*  socket address structure  */
  char      inbuf[MAX_LINE];       /*  character inbuf          */
  char      outbuf[MAX_LINE];      /*  character outbuf          */
  char     *endptr;                /*  for strtol()              */
  int       acceptFlag = 0;
  int       closeFlag = 0;
  ssize_t   rv;
  int       cmd;
  int       nfds;
  fd_set    notify_set;
  workCmd_t workCmd;
  workCmd_t workReply;
  bool      singleWorker = false;
  bool helpFlag = false;
  bool errorFlag = false;
//char *endPtr;
//extern char* optarg;

  int c;
  while( ( c = getopt( argc, argv, "vhs" ) ) != EOF ) {
    switch(c) {
      case 'h':
        helpFlag = true;
        break;
      case 'v':
        ++_verbosity;
        break;
      case 's':
        _simFlag = true;
        break;
      default:
        errorFlag = true;
        break;
    }
  }

  if (helpFlag) {
    usage(argv[0]);
    exit(0);
  }

  if (errorFlag) {
    usage(argv[0]);
    exit(1);
  }

  /* init logging */
  switch (_verbosity) {
    case 0:
      rnxlogInit(LEVEL_NOTICE, argv[0]);
      break;
    case 1:
      rnxlogInit(LEVEL_INFO, argv[0]);
      break;
    default:
      rnxlogInit(LEVEL_DEBUG, argv[0]);
      break;
  }

  /* advertize simulation mode */
  if (_simFlag) {
    char lilbuf[200];
    sprintf(lilbuf, "%s: SIMULATION flag is set", argv[0]);
    printf("%s\n", lilbuf);
    INFO_LOG(lilbuf);
  }

  /* init frame count and semaphore */
  frameCountInit();

  /* init config semaphore */
  configSemInit();

  /* create work pipe */
  if (pipe(_workPipeFd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  /* create control pipe */
  if (pipe(_controlPipeFd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  rnxReset(false);    /* reset state (only after pipes initialized) */

  if (create_threads(singleWorker) != 0) {
    fprintf(stderr, "%s: Error creating threads.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* create listening socket */
  if ((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    fprintf(stderr, "%s: Error creating listening socket.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*  Set all bytes in socket address structure to
      zero, and fill in the relevant data members   */
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(port);

  /* Enable address reuse */
  int on = 1;
  if (setsockopt( list_s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    perror("setsockopt");
    ERROR_LOG("Error setting socket SO_REUSEADDR option");
    exit(EXIT_FAILURE);
  }

  /*  Bind our socket addresss to the 
      listening socket, and call listen()  */
  if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
      perror("bind");
      ERROR_LOG("Error binding command socket");
      exit(EXIT_FAILURE);
  }

  if ( listen(list_s, LISTENQ) < 0 ) {
      perror("listen");
      ERROR_LOG("Error listening command socket");
      exit(EXIT_FAILURE);
  }

  // We expect write failures to occur but we want to handle them where 
  // the error occurs rather than in a SIGPIPE handler.
  signal(SIGPIPE, SIG_IGN);

  printf("%s: waiting for commands on port %d ...\n", argv[0], port);

  /*  Enter an infinite loop to respond to client requests */
  while ( 1 ) {
    /*  Wait for a connection, then accept() it  */
    if (!acceptFlag) {
        if ( (conn_s = accept(list_s, NULL, NULL) ) < 0 ) {
            perror("accept");
            ERROR_LOG("Error accepting command socket");
            exit(EXIT_FAILURE);
        } else {
            acceptFlag = 1;
            INFO_LOG("Command socket opened");
            sprintf(outbuf, "%s", RNX_REPLY_GREETING);
            Writeline(conn_s, outbuf, strlen(outbuf));
        }
    }

    /* select */
    FD_ZERO(&notify_set);
    FD_SET(conn_s, &notify_set);
    FD_SET(_controlPipeFd[0], &notify_set);
    nfds = (conn_s > _controlPipeFd[0]) ? conn_s+1 : _controlPipeFd[0]+1;

    if (select(nfds, &notify_set, NULL, NULL, NULL) == -1) {
      perror("select");
      sleep(1); continue;
    }

    if (FD_ISSET(_controlPipeFd[0], &notify_set)) {
      /* read from pipe */
      int length = read(_controlPipeFd[0], &workReply, sizeof(workReply));
      if (length == -1) {
        perror("read");
        ERROR_LOG("Failed read() on pipe");
      } else if (workReply.epoch != _rnxEpoch) {
        ERROR_LOG("received control pipe cmd from earlier epoch - ignored");
        outbuf[0] = '\0'; /* no reply */
      } else {
        char lilbuf[200];
        switch (workReply.cmd) {
          case 2: /* config ok */
            _rnxState = RNX_STATE_CONFIGURED;
            INFO_LOG("state = RNX_STATE_CONFIGURED");
            _framesize = workReply.arg1;
            _readoutMode = workReply.arg2;
            _testPattern = workReply.arg3;
            strncpy(_deviceID, workReply.argv, MAX_DEVICEID);
            sprintf(lilbuf, "framesize=%d;epoch=%d;deviceID=%s", _framesize, _rnxEpoch, _deviceID);
            INFO_LOG(lilbuf);
            rnxStateReply(outbuf);
            Writeline(conn_s, outbuf, strlen(outbuf));
            break;
          case 5: /* config error */
            Writeline(conn_s, RNX_REPLY_FAILED, strlen(RNX_REPLY_FAILED));
            /* reset after config error, to avoid partially configured state */
            ERROR_LOG("config error - calling rnxReset()");
            rnxReset(false);
            break;
          case 12: /* dark ok */
            Writeline(conn_s, RNX_REPLY_OK, strlen(RNX_REPLY_OK));
            INFO_LOG("dark OK");
            break;
          case 15: /* dark error */
            Writeline(conn_s, RNX_REPLY_FAILED, strlen(RNX_REPLY_FAILED));
            ERROR_LOG("dark error");
            break;
          default:
            sprintf(lilbuf, "received unknown workReply = %d\n", workReply.cmd);
            ERROR_LOG(lilbuf);
            break;
        }
      }
    }

    if (FD_ISSET(conn_s, &notify_set)) {
      /* read command */
      rv = Readline(conn_s, inbuf, MAX_LINE-1);
      cmd = translate(inbuf);

      /* interpret command */
      if (rv == 0) {
        /* read command returned 0 -- close now */
        closeFlag = 1;
        goto close_now;
      } else {
        cmd = translate(inbuf);
      }

      switch (cmd) {

        case RNX_CMD_NOOP:
          /* no operation except reply OK */
          sprintf(outbuf, "%s", RNX_REPLY_OK);
          break;

        case RNX_CMD_QUIT:
          closeFlag = 1;  /* close later, after replying */
          sprintf(outbuf, "%s", RNX_REPLY_QUIT);
          break;

        case RNX_CMD_BULB:
          if (_rnxState == RNX_STATE_ENABLED) {
            /* sw trigger */
            workCmd.cmd = RNX_WORK_SWTRIGGER;
            workCmd.epoch = _rnxEpoch;
            if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
              perror("write");
              ERROR_LOG("error writing to work pipe");
            }
            sprintf(outbuf, "%s", RNX_REPLY_BULB);
            INFO_LOG("PulseBulb()");
          } else {
            sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
            ERROR_LOG("bad sequence of commands: bulb while not RNX_STATE_ENABLED");
          }
          break;

        case RNX_CMD_HELLO:
          rnxReset(false);    /* reset state */
          sprintf(outbuf, "%s", RNX_REPLY_HELO);
          break;

        case RNX_CMD_CONFIG:
          switch (_rnxState) {
            case RNX_STATE_ENABLED:
              sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
              ERROR_LOG("bad sequence of commands: conf while RNX_STATE_ENABLED");
              break;
            case RNX_STATE_CONFIGURED:
              sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
              ERROR_LOG("bad sequence of commands: conf while RNX_STATE_CONFIGURED");
              break;
            case RNX_STATE_UNCONFIGURED:
              workCmd.cmd = RNX_WORK_OPENDEV;
              workCmd.epoch = _rnxEpoch;
              /* skip initial space char, if any */
              endptr = index(inbuf, ' ');
              if (endptr) {
                sprintf(workCmd.argv, "%s", endptr+1);
              } else {
                workCmd.argv[0] = '\0';
              }
              /* open device */
              if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
                perror("write");
                ERROR_LOG("error writing to work pipe");
              }
              /* do not change state yet.  reply will come via controlPipe */
              outbuf[0] = '\0'; /* no reply */
              break;
            default:
              ERROR_LOG("invalid state");
              sprintf(outbuf, "%s", RNX_REPLY_STATEINV);
              break;
          }
          break;

        case RNX_CMD_DARK:
          if (_rnxState == RNX_STATE_CONFIGURED) {
            /* start acquisition */
            workCmd.cmd = RNX_WORK_DARK;
            workCmd.epoch = _rnxEpoch;
            if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
              perror("write");
              ERROR_LOG("error writing to work pipe");
            }
            /* reply will come via controlPipe */
            outbuf[0] = '\0'; /* no reply */
          } else {
            /* ERROR reply (bad sequence) */
            sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
            if (_rnxState == RNX_STATE_ENABLED) {
              ERROR_LOG("bad sequence of commands: dark while RNX_STATE_ENABLED");
            } else {
              ERROR_LOG("bad sequence of commands: dark while RNX_STATE_UNCONFIGURED");
            }
          }
          break;

        case RNX_CMD_ENABLE:
          if (_rnxState == RNX_STATE_CONFIGURED) {
            /* start acquisition */
            workCmd.cmd = RNX_WORK_STARTACQ;
            workCmd.epoch = _rnxEpoch;
            if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
              perror("write");
              ERROR_LOG("error writing to work pipe");
            }
            _rnxState = RNX_STATE_ENABLED;
            rnxStateReply(outbuf);
            INFO_LOG("state = RNX_STATE_ENABLED");
          } else {
            /* ERROR reply (bad sequence) */
            sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
            if (_rnxState == RNX_STATE_ENABLED) {
              ERROR_LOG("bad sequence of commands: enab while RNX_STATE_ENABLED");
            } else {
              ERROR_LOG("bad sequence of commands: enab while RNX_STATE_UNCONFIGURED");
            }
          }
          break;

        case RNX_CMD_DISABLE:
          if (_rnxState == RNX_STATE_ENABLED) {
            /* end acquisition */
            workCmd.cmd = RNX_WORK_ENDACQ;
            workCmd.epoch = _rnxEpoch;
            if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
              perror("write");
              ERROR_LOG("error writing to work pipe");
            }
            _rnxState = RNX_STATE_CONFIGURED;
            INFO_LOG("state = RNX_STATE_CONFIGURED");
            rnxStateReply(outbuf);
          } else {
            /* ERROR reply (bad sequence) */
            sprintf(outbuf, "%s", RNX_REPLY_BADSEQ);
            if (_rnxState == RNX_STATE_CONFIGURED) {
              ERROR_LOG("bad sequence of commands: disa while RNX_STATE_CONFIGURED");
            } else {
              ERROR_LOG("bad sequence of commands: disa while RNX_STATE_UNCONFIGURED");
            }
          }
          break;

        case RNX_CMD_HELP:
          sprintf(outbuf, "%s", RNX_REPLY_NOTIMPL);
          break;

        case RNX_CMD_LOGS:
          rnxLogReply(outbuf);
          break;

        case RNX_CMD_STAT:
          rnxStateReply(outbuf);
          break;

        case RNX_CMD_RESET:
          rnxReset(false);    /* reset state */
          sprintf(outbuf, "%s", RNX_REPLY_OK);
          break;

        default:
          /* command not recognized */
          sprintf(outbuf, "%s", RNX_REPLY_NORECOG);
          break;
      }

      if (strlen(outbuf) > 0) {
        Writeline(conn_s, outbuf, strlen(outbuf));
      }

close_now:
      if (closeFlag) {
        if (_rnxState != RNX_STATE_UNCONFIGURED) {
          /* reset state */
          rnxReset(false);
        }
        /*  Close the connected socket  */
        if ( close(conn_s) < 0 ) {
          perror("close");
          ERROR_LOG("Error closing command socket");
          exit(EXIT_FAILURE);
        } else {
          INFO_LOG("Command socket closed");
          closeFlag = 0;  /* done closing */
          acceptFlag = 0; /* reenable accept() */
        }
      }
    }
  }
}

void rnxStateReply(char *buf)
{
  if (buf) {
    switch (_rnxState) {
      case 3:
        sprintf(buf, RNX_REPLY_STATE3, _framesize, _rnxEpoch, _deviceID, _readoutMode, _testPattern);
        break;
      case 2:
        sprintf(buf, RNX_REPLY_STATE2, _framesize, _rnxEpoch, _deviceID, _readoutMode, _testPattern);
        break;
      case 1:
        sprintf(buf, RNX_REPLY_STATE1);
        break;
      default:
        sprintf(buf, "%s", RNX_REPLY_STATEINV);
        break;
    }
  }
}

void rnxLogReply(char *buf)
{
  if (buf) {
    switch (_rnxLogLevel) {
      case 7:
        sprintf(buf, "%s", RNX_REPLY_LOG7);
        break;
      case 6:
        sprintf(buf, "%s", RNX_REPLY_LOG6);
        break;
      case 5:
        sprintf(buf, "%s", RNX_REPLY_LOG5);
        break;
      case 4:
        sprintf(buf, "%s", RNX_REPLY_LOG4);
        break;
      case 3:
        sprintf(buf, "%s", RNX_REPLY_LOG3);
        break;
      default:
        sprintf(buf, "%s", RNX_REPLY_LOGINV);
        break;
    }
  }
}

void rnxReset(bool closeDevice)
{
  workCmd_t workCmd;

  /* advance epoch counter (to detect replies from old commands) */
  _rnxEpoch++;
  workCmd.cmd = RNX_WORK_CLOSEDEV;
  workCmd.epoch = _rnxEpoch;

  if (_rnxState == RNX_STATE_ENABLED) {
    /* end acquisition */
    workCmd.cmd = RNX_WORK_ENDACQ;
    workCmd.epoch = _rnxEpoch;
    if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
      perror("write");
      ERROR_LOG("error writing to work pipe");
    }
  }

  if (closeDevice) {
    /* close device */
    if (write(_workPipeFd[1], &workCmd, sizeof(workCmd)) == -1) {
      perror("write");
      ERROR_LOG("error writing to work pipe");
    } else {
      printf("%s: sent RNX_WORK_CLOSEDEV to work thread (epoch=%d)\n", __FUNCTION__, workCmd.epoch);
    }
  }

  /* TODO more may be needed */

  if (_rnxState != RNX_STATE_UNCONFIGURED) {
    _rnxState = RNX_STATE_UNCONFIGURED;
    INFO_LOG("state = RNX_STATE_UNCONFIGURED");
  }
  _rnxLogLevel = 7;
  frameCountReset();
}

int translate(char *buf)
{
  int rv = 0;
  if ((buf) && (strlen(buf) >= 4)) {
    rv = ((tolower(buf[0]) << 24) +
          (tolower(buf[1]) << 16) +
          (tolower(buf[2]) <<  8) +
          (tolower(buf[3]) <<  0));
  }
  return (rv);
}

int create_threads(bool singleWorker)
{
  pthread_t workThreads[NUM_WORK_THREADS];
  work_state_t *workState[NUM_WORK_THREADS];
  pthread_t triggerThread;
  trigger_state_t *triggerState;
  int rc, t;

  /* initialize thread states */
  workState[0] = (work_state_t *)malloc(sizeof(work_state_t));
  workState[0]->task_id = 0;
  workState[0]->work_pipe_fd = _workPipeFd[0];
  workState[0]->control_pipe_fd = _controlPipeFd[1];
  workState[0]->pConfigSem = &configSem;
  workState[0]->simFlag = _simFlag;

  workState[1] = (work_state_t *)malloc(sizeof(work_state_t));
  workState[1]->task_id = 1;
  workState[1]->work_pipe_fd = _workPipeFd[0];
  workState[1]->control_pipe_fd = _controlPipeFd[1];
  workState[1]->pConfigSem = &configSem;
  workState[1]->simFlag = _simFlag;

  triggerState = (trigger_state_t *)malloc(sizeof(trigger_state_t));
  triggerState->task_id = 2;
  triggerState->write_pipe_fd = _workPipeFd[1];
  triggerState->mode = RNX_TRIGGER_MODE_UDP;    /* modes: timer or udp */

  /* create threads */
  int numWorkThreads = singleWorker ? 1 : NUM_WORK_THREADS;
  for(t=0; t < numWorkThreads; t++) {
    rc = pthread_create(&workThreads[t], NULL, workRoutine, (void *)(workState[t]));
    if (rc) {
      perror("pthread_create");
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      return (-1);
    }
  }
  rc = pthread_create(&triggerThread, NULL, triggerRoutine, (void *)(triggerState));
  if (rc) {
    perror("pthread_create");
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    return (-1);
  }


  return (0);
}

int rnxState() { return _rnxState; }
unsigned int rnxEpoch() { return _rnxEpoch; }
int rnxLogLevel() { return _rnxLogLevel; }

static uint16_t frameCount;   /* frame counter */
static sem_t hwSem;           /* semaphore protects frame counter */

int configSemInit()
{
  if (sem_init(&configSem, 0, 1) == -1) {
    perror("sem_init configSem");
    return (-1);
  }
  return 0;
}

int frameCountInit()
{
  if (sem_init(&hwSem, 0, 1) == -1) {
    perror("sem_init hwSem");
    return (-1);
  }
  frameCount = 0;
  return 0;
}

int frameCountReset()
{
  if (sem_wait(&hwSem) == -1) {
    perror("sem_wait");
  }
  frameCount = 0;
  if (sem_post(&hwSem) == -1) {
    perror("sem_post");
  }
  return (0);
}

int frameCountGet()
{
  int rv;
  if (sem_wait(&hwSem) == -1) {
    perror("sem_wait");
  }
  rv = frameCount;
  if (sem_post(&hwSem) == -1) {
    perror("sem_post");
  }
  return (rv);
}

int frameCountAdvance()
{
  int rv;
  if (sem_wait(&hwSem) == -1) {
    perror("sem_wait");
  }
  rv = ++frameCount;
  if (sem_post(&hwSem) == -1) {
    perror("sem_post");
  }
  return (rv);
}
