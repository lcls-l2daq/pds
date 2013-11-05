#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "workThread.h"
#include "rnxdata.h"
#include "rnxlog.h"
#include "rnxcommon.h"
#include "rnxState.h"

#include "craydl.h"
#include "RxDetector.h"

#define DEST_IPADDR         "10.0.1.1"
#define NOTIFY_PORT         30050
// #define SIM_NOTIFY_PORT  30057
// #define DATA_PORT        30000

#define MAX_LINE_PIXELS     3840
#define LINEBUF_WORDS       4000    /* enough for data footer + max line (3840 16-bit pixels) */

#define UDP_SNDBUF_SIZE     (64*1024*1024)

/* limit 1 config per epoch */
static unsigned int _lastConfigEpoch = (unsigned int)-1;
static int _binning_f = 2;
static int _binning_s = 2;
static double _exposure = 0.0;
static int _testPattern = 0;
static bool _rawFlag = false;
static bool _darkFlag = false;
static craydl::ReadoutMode_t  _readoutMode = craydl::RM_Standard;

// forward declaration
int sendFrame(int frameNumber, uint16_t timeStamp, uint16_t *frame);

// All craydl classes, functions, etc, reside in the craydl namespace
using namespace craydl;
using std::string;

class FrameHandlerCB : public VirtualFrameCallback
{
   public:
      FrameHandlerCB(RxDetector& detector) :
         detector_(detector),
	       multi_sequence_(false),
         sequence_number_(0),
         sequence_number_width_(2),
         sequence_seperator_("_"),
         write_frames(false),
         frame_number_width_(4)
      {
      }
      ~FrameHandlerCB() {}
      void SequenceStarted() {RxLog(LOG_DEBUG) << "CB: Sequence Started ()" << std::endl;}
      void SequenceEnded() {RxLog(LOG_DEBUG) << "CB: Sequence Ended ()" << std::endl;}
      void ExposureStarted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Exposure " << frame_number << " started." << std::endl;}
      void ExposureEnded(int frame_number) {RxLog(LOG_DEBUG) << "CB: Exposure " << frame_number << " ended." << std::endl;}
      void ReadoutStarted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Readout " << frame_number << " started." << std::endl;}
      void ReadoutEnded(int frame_number) {RxLog(LOG_DEBUG) << "CB: Readout " << frame_number << " ended." << std::endl;}
//    void BackgroundFrameReady(RxFrame *frame_p) { RxLog(LOG_DEBUG) << "CB: Background Frame " << " buffer address: " << frame_p->getBufferAddress() << std::endl; }
      void BackgroundFrameReady(const RxFrame *frame_p) { RxLog(LOG_DEBUG) << "CB: Background Frame " << " buffer address: " << frame_p->getBufferAddress() << std::endl; }

      void RawFrameReady(int frame_number, const RxFrame *frame_p)
      {
         if (_rawFlag) {
           boost::posix_time::ptime ppp = frame_p->constMetaData().HardwareTimestamp();
           boost::posix_time::time_duration duration( ppp.time_of_day() );
           unsigned short timeStamp = duration.total_milliseconds() % 0xffff;

           printf("%s: Raw Frame #%d  size=%u  timestamp=%hu ms\n", __FUNCTION__, frame_number, (unsigned)frame_p->getSize(), timeStamp);

           sendFrame(frame_number, timeStamp, (uint16_t *)(frame_p->getBufferAddress()));
         }
         return;
      }

      void FrameReady(int frame_number, const RxFrame *frame_p)
      {
         if (!_rawFlag) {
           boost::posix_time::ptime ppp = frame_p->constMetaData().HardwareTimestamp();
           boost::posix_time::time_duration duration( ppp.time_of_day() );
           unsigned short timeStamp = duration.total_milliseconds() % 0xffff;

           printf("%s: Frame #%d  size=%u  timestamp=%hu ms\n", __FUNCTION__, frame_number, (unsigned)frame_p->getSize(), timeStamp);

           sendFrame(frame_number, timeStamp, (uint16_t *)(frame_p->getBufferAddress()));
         }
         return;
      }

      void FrameAborted(int frame_number) {RxLog(LOG_WARNING) << "CB: Frame " << frame_number << " aborted." << std::endl;}
      void FrameCompleted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Frame " << frame_number << " completed." << std::endl;}
      void FrameError(int frame_number, const int error_code, const std::string& error_string) {RxLog(LOG_ERROR) << "CB: Frame " << frame_number << " reported error " << error_code << ":" << error_string << std::endl;}

      void set_write_frames(const bool write_frames = true) {
      }

   private:

      RxDetector& detector_;
      bool multi_sequence_;
      int sequence_number_;
      int sequence_number_width_;
      std::string sequence_seperator_;
      bool write_frames;
      int frame_number_width_;
};

RxDetector *openDetector(const char *configFileName, const char *detectorName);
int closeDetector(RxDetector *pDetector);
int decodeConfigArgs(char *argstr, int *binning_f, int *binning_s, int *exposure_ms, bool *rawFlag, bool *darkFlag, int *testPattern, int *readoutMode, int *triggerMode);

static uint16_t linebuf0[LINEBUF_WORDS * 2];
static uint16_t linebuf1[LINEBUF_WORDS * 2];

static RxDetector *pDetector = NULL;
static VirtualFrameCallback* pFrameCallback = NULL;

static int notifyFd, dataFdEven, dataFdOdd;
static struct sockaddr_in notifyaddr, dataaddreven, dataaddrodd;

void *workRoutine(void *arg)
{
  work_state_t *myState = (work_state_t *)arg;
  int tid = myState->task_id;
  workCmd_t  workCommand;
  workCmd_t  workReply;
  data_footer_t notifyMsg;
  char logbuf[LOGBUF_SIZE];
  char logbuf2[LOGBUF_SIZE];
  uint16_t *linebuf = (tid == 0) ? linebuf0 : linebuf1;
  int writeSize, sent;
  RxReturnStatus error;

  sprintf(logbuf, "Thread #%d started", tid);
  DEBUG_LOG(logbuf);

  /* open notification socket */
  notifyFd = socket(AF_INET,SOCK_DGRAM, 0);
  if (notifyFd == -1) {
    perror("socket");
    ERROR_LOG("Failed to open notification socket");
    pthread_exit(NULL);
  }
  DEBUG_LOG("Notification socket open");

  /* open data sockets */
  dataFdEven = socket(AF_INET,SOCK_DGRAM, 0);
  if (dataFdEven == -1) {
    perror("socket");
    ERROR_LOG("Error: Failed to open even data socket");
    pthread_exit(NULL);
  }
  dataFdOdd = socket(AF_INET,SOCK_DGRAM, 0);
  if (dataFdOdd == -1) {
    perror("socket");
    ERROR_LOG("Error: Failed to open odd data socket");
    pthread_exit(NULL);
  }
  DEBUG_LOG("Data sockets open");

  /* init notifyMsg */
  bzero(&notifyMsg, sizeof(notifyMsg));

  /* init notifyaddr */
  bzero(&notifyaddr, sizeof(notifyaddr));
  notifyaddr.sin_family = AF_INET;
  notifyaddr.sin_addr.s_addr=inet_addr(DEST_IPADDR);
  notifyaddr.sin_port=htons(NOTIFY_PORT);

  /* init dataaddreven */
  bzero(&dataaddreven, sizeof(dataaddreven));
  dataaddreven.sin_family = AF_INET;
  dataaddreven.sin_addr.s_addr=inet_addr(DEST_IPADDR);
  dataaddreven.sin_port=htons(RNX_DATA_PORT_EVEN);

  /* init dataaddrodd */
  bzero(&dataaddrodd, sizeof(dataaddrodd));
  dataaddrodd.sin_family = AF_INET;
  dataaddrodd.sin_addr.s_addr=inet_addr(DEST_IPADDR);
  dataaddrodd.sin_port=htons(RNX_DATA_PORT_ODD);

  while (1) {
    /* read from pipe */
    sprintf(logbuf, "Thread #%d: Reading from pipe", tid);
    DEBUG_LOG(logbuf);
    int length = read(myState->work_pipe_fd, &workCommand, sizeof(workCommand));
    if (length == -1) {
      perror("read");
      ERROR_LOG("Failed read() on pipe");
      sleep (1);
      continue;   /* try again */
    }

    logbuf2[0] = '\0';
    if (length == sizeof(workCommand)) {
      switch (workCommand.cmd) {
        case  RNX_WORK_FRAMEREADY:
          sprintf(logbuf2, "Thread #%d: FRAMEREADY\n", tid);
          break;
        case  RNX_WORK_OPENDEV:
          sprintf(logbuf2, "Thread #%d: OPENDEV\n", tid);
          break;
        case  RNX_WORK_CLOSEDEV:
          sprintf(logbuf2, "Thread #%d: CLOSEDEV\n", tid);
          break;
        case  RNX_WORK_STARTACQ:
          sprintf(logbuf2, "Thread #%d: STARTACQ\n", tid);
          break;
        case  RNX_WORK_ENDACQ:
          sprintf(logbuf2, "Thread #%d: ENDACQ\n", tid);
          break;
        default:
          sprintf(logbuf2, "Thread #%d: cmd=%d arg1=%d epoch=%u\n", tid,
                  workCommand.cmd, workCommand.arg1, workCommand.epoch);
          break;
      }
      DEBUG_LOG(logbuf2);
    } else {
      printf("read() returned %d\n", length);
      ERROR_LOG("work command wrong size");
    }

    workReply.epoch = workCommand.epoch;    /* set up reply with same epoch as command */

    if (workCommand.cmd == RNX_WORK_OPENDEV) {
      int binning_f = 2;
      int binning_s = 2;
      int exposure_ms = 0;
      bool rawFlag = false;
      bool darkFlag = false;
      bool simFlag = false;
      int testPattern = 0;
      int readoutMode = craydl::RM_Standard;
      int triggerMode = 0;
      int fast, slow, depth;
      int errorCount = 0;
      /* interpret arguments */
      if (workCommand.arg1 != 0) {
        simFlag = true;
        snprintf(logbuf, sizeof(logbuf), "%s: Simulation flag is set", __FUNCTION__);
        printf("%s\n", logbuf);
        INFO_LOG(logbuf);
      }
      if (decodeConfigArgs(workCommand.argv, &binning_f, &binning_s, &exposure_ms, &rawFlag, &darkFlag, &testPattern, &readoutMode, &triggerMode) == -1) {
        snprintf(logbuf, sizeof(logbuf), "%s: decodeConfigArgs() returned -1", __FUNCTION__);
        printf("%s\n", logbuf);
        ERROR_LOG(logbuf);
        workReply.cmd = 5;  /* error */
        if (write(myState->control_pipe_fd, &workReply, sizeof(workReply)) == -1) {
          perror("write");
          ERROR_LOG("error writing to control pipe");
        }
        continue;   /* did not take semaphore */
      }

      /* semaphore take */
      if (sem_wait(myState->pConfigSem) == -1) {
        perror("sem_wait pConfigSem");
        ERROR_LOG("sem_wait() failed");
      }

      if (rnxEpoch() != workCommand.epoch) {
        ERROR_LOG("epoch changed before configuration - skipping config");
      } else if (rnxEpoch() == _lastConfigEpoch) {
        ERROR_LOG("duplicate config for single epoch - skipping config");
      } else {
        if (!pDetector) {
          /* if not already open... */
          pDetector = openDetector(CONFIG_FILE_NAME, simFlag ? DETECTOR_NAME_SIM : DETECTOR_NAME_HW);
        }
        if (!pDetector) {
          snprintf(logbuf, sizeof(logbuf), "%s: openDetector(%s, %s) returned NULL", __FUNCTION__,
                   CONFIG_FILE_NAME, simFlag ? DETECTOR_NAME_SIM : DETECTOR_NAME_HW);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
          errorCount++;
          workReply.cmd = 5;
          if (write(myState->control_pipe_fd, &workReply, sizeof(workReply)) == -1) {
            perror("write");
            ERROR_LOG("error writing to control pipe");
            errorCount++;
          }
        } else {
          _lastConfigEpoch = workCommand.epoch; /* 1 config per epoch */
          DEBUG_LOG("openDetector() done");
          if (rnxEpoch() != workCommand.epoch) {
            ERROR_LOG("epoch changed during configuration");
            errorCount++;
          } else if ((binning_f < RNX_BINNING_MIN) || (binning_f > RNX_BINNING_MAX)) {
            sprintf(logbuf, "Error: binning_f %d is outsize valid range %d-%d",
                    binning_f, RNX_BINNING_MIN, RNX_BINNING_MAX);
            printf("%s\n", logbuf);
            ERROR_LOG(logbuf);
            errorCount++;
          } else {
            /* set binning */
            error = pDetector->SetBinning(binning_f); /* TODO support binning_s */
            if (error.IsError()) {
              sprintf(logbuf, "Error: SetBinning(%d) failed", binning_f);
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              _binning_f = binning_f;
              _binning_s = binning_s;
              if (_binning_f != _binning_s) {
                sprintf(logbuf, "Warning: binning_s (%d) differs from binning_f (%d), binning_s ignored", _binning_s, _binning_f);
                printf("%s\n", logbuf);
                WARNING_LOG(logbuf);
              }
              sprintf(logbuf, "SetBinning(%d) done", _binning_f);
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }

            /* set trigger mode */
            error = pDetector->SetFrameTriggerMode(std::string("Frame"));
            if (error.IsError()) {
              sprintf(logbuf, "Error: SetTriggerMode(Frame) failed");
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              sprintf(logbuf, "SetTriggerMode(Frame) done");
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }

            /* set sequence gate */
            error = pDetector->SetSequenceGate(std::string("None"));
            if (error.IsError()) {
              sprintf(logbuf, "Error: SetSequenceGate(None) failed");
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              sprintf(logbuf, "SetSequenceGate(None) done");
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }

            /* set exposure */
            _exposure = exposure_ms / 1000.0;
            error = pDetector->SetExposureTime(_exposure);
            if (error.IsError()) {
              sprintf(logbuf, "Error: SetExposureTime(%5.3f) failed", _exposure);
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              sprintf(logbuf, "SetExposureTime(%5.3f) done", _exposure);
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }

            // Check - will print to log file
            // ? pDetector->TriggerType();

            /* set modes */
            _rawFlag = rawFlag;
            _darkFlag = darkFlag;
            switch (readoutMode) {
              case RM_HighGain:
                _readoutMode = RM_HighGain;
                break;
              case RM_HDR:
                _readoutMode = RM_HDR;
                break;
              default:
                sprintf(logbuf, "Warning: Readout mode %d not supported", (int) readoutMode);
                printf("%s\n", logbuf);
                WARNING_LOG(logbuf);
                /* fall through */
              case RM_Standard:
                _readoutMode = RM_Standard;
                break;
            }
            error = pDetector->SetReadoutMode(_readoutMode);
            if (error.IsError()) {
              sprintf(logbuf, "Error: SetReadoutMode(%d) failed", (int) _readoutMode);
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              sprintf(logbuf, "SetReadoutMode(%d) done", (int) _readoutMode);
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }

            /* set test pattern */
            if (testPattern) {
              error = pDetector->SetTestPattern((TestPattern_t)testPattern);
              if (error.IsError()) {
                sprintf(logbuf, "SetTestPattern(%d) failed", testPattern);
                printf("%s\n", logbuf);
                ERROR_LOG(logbuf);
                errorCount++;
              } else {
                sprintf(logbuf, "SetTestPattern(%d) done", testPattern);
                printf("%s\n", logbuf);
                INFO_LOG(logbuf);
                _testPattern = testPattern;
                error = pDetector->UseTestPattern(true);
                if (error.IsError()) {
                  sprintf(logbuf, "UseTestPattern(true) failed");
                  printf("%s\n", logbuf);
                  ERROR_LOG(logbuf);
                  errorCount++;
                } else {
                  sprintf(logbuf, "UseTestPattern(true) done");
                  printf("%s\n", logbuf);
                  INFO_LOG(logbuf);
                }
              }
            } else {
              error = pDetector->UseTestPattern(false);
              if (error.IsError()) {
                sprintf(logbuf, "UseTestPattern(false) failed");
                printf("%s\n", logbuf);
                ERROR_LOG(logbuf);
                errorCount++;
              } else {
                _testPattern = 0;
                sprintf(logbuf, "UseTestPattern(false) done");
                printf("%s\n", logbuf);
                INFO_LOG(logbuf);
              }
            }

            /* send parameters */
            error = pDetector->SendParameters();
            if (error.IsError()) {
              sprintf(logbuf, "Error: SendParameters() failed");
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              sprintf(logbuf, "SendParameters() done");
              printf("%s\n", logbuf);
              INFO_LOG(logbuf);
            }
          }

          /* command: 5=Error, 2=OK */
          workReply.cmd = (errorCount > 0) ? 5 : 2;

          if (workReply.cmd == 2) {
            workReply.arg1 = pDetector->GetFrameSize(fast, slow, depth);
            std::string name, number;
            pDetector->GetDetectorID(name, number);
            sprintf(workReply.argv, "%s:%s", name.c_str(), number.c_str());
          } else {
            workReply.arg1 = 0;   /* ERROR */
          }

          printf("%swork%d task sending cmd <%d> framesize <%d> epoch <%u> detectorID <%s>\n",
                 myState->task_id ? "  " : "", myState->task_id, workReply.cmd,
                 workReply.arg1, workReply.epoch, workReply.argv);

          if (write(myState->control_pipe_fd, &workReply, sizeof(workReply)) == -1) {
            perror("write");
            ERROR_LOG("error writing to control pipe");
          }
        }
      }
      /* semaphore give */
      if (sem_post(myState->pConfigSem) == -1) {
        perror("sem_post pConfigSem");
        ERROR_LOG("sem_post() failed");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_CLOSEDEV) {
      /* check epoch */
      if (rnxEpoch() != workCommand.epoch) {
        DEBUG_LOG("RNX_WORK_CLOSEDEV ignored - epoch changed");
        continue;
      }
      /* semaphore take */
      if (sem_wait(myState->pConfigSem) == -1) {
        perror("sem_wait pConfigSem");
        ERROR_LOG("sem_wait() failed");
      }
      if (pDetector) {
        if (closeDetector(pDetector) == 0) {
          DEBUG_LOG("closeDetector() done");
        } else {
          ERROR_LOG("closeDetector() failed");
        }
        pDetector = NULL;
      } else {
        DEBUG_LOG("RNX_WORK_CLOSEDEV ignored - device not open");
      }
      /* semaphore give */
      if (sem_post(myState->pConfigSem) == -1) {
        perror("sem_post pConfigSem");
        ERROR_LOG("sem_post() failed");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_FRAMEREADY) {
      int ii, jj;
      data_footer_t *pFooter;

      /* simulate frame counter */
      notifyMsg.frameNumber = frameCountAdvance();
      notifyMsg.damage = 0;
      notifyMsg.epoch = _lastConfigEpoch;
      notifyMsg.binning_f = _binning_f;
      notifyMsg.binning_s = _binning_s;
      notifyMsg.pad = 0;

      /* simulate frame from detector */
      for (ii = 0; ii < MAX_LINE_PIXELS; ii++) {
        linebuf[ii] = notifyMsg.frameNumber;  /* note: uint16_t frameNumber */
      }

      /* add footer */
      pFooter = (data_footer_t *)(linebuf + ii);
      pFooter->cmd = 0;
      pFooter->frameNumber = notifyMsg.frameNumber;
      pFooter->damage = 0;
      pFooter->epoch = _lastConfigEpoch;
      pFooter->binning_f = _binning_f;
      pFooter->binning_s = _binning_s;
      pFooter->pad = 0;

      /* send lines */
      sprintf(logbuf, "Thread #%d: Send frame #%d lines...\n", tid, notifyMsg.frameNumber);
      DEBUG_LOG(logbuf);
      writeSize = (MAX_LINE_PIXELS * 2) + sizeof(data_footer_t);
      for (jj = 0; jj < MAX_LINE_PIXELS/_binning_s; jj += _binning_f) {
        /* update data footer */
        pFooter->lineNumber = jj;

        /* write to data socket */
        if (notifyMsg.frameNumber & 1) {
          // odd frame -> odd data port
          sent = sendto(dataFdOdd, (void *)linebuf, writeSize, 0,
                     (struct sockaddr *)&dataaddrodd, sizeof(dataaddrodd));
        } else {
          // even frame -> even data port
          sent = sendto(dataFdEven, (void *)linebuf, writeSize, 0,
                     (struct sockaddr *)&dataaddreven, sizeof(dataaddreven));
        }
        if (sent == -1) {
          perror("sendto");
          sprintf(logbuf, "Thread #%d: Frame #%d sendto() failed (errno=%d)",
                  tid, notifyMsg.frameNumber, errno);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
          break;
        } else if (sent != writeSize) {
          sprintf(logbuf, "Error: Thread #%d: Sent %d of %d bytes for Frame #%d Line #%d",
                  tid, sent, writeSize, notifyMsg.frameNumber, jj);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        }
      }

      /* write to notification socket */
      sprintf(logbuf, "Thread #%d: Send frame #%d notification...", tid, notifyMsg.frameNumber);
      DEBUG_LOG(logbuf);
      sent = sendto(notifyFd, (void *)&notifyMsg, sizeof(notifyMsg), 0,
                 (struct sockaddr *)&notifyaddr, sizeof(notifyaddr));
      if (sent == -1) {
        perror("sendto");
        ERROR_LOG("Failed sendto() on notify socket");
      } else if (sent != sizeof(notifyMsg)) {
        printf("sent %d of %lu bytes\n", sent, sizeof(notifyMsg));
        ERROR_LOG("sendto() notify socket returned incorrect size");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_STARTACQ) {
      // RxFrameAcquisitionType frame_type;
      FrameAcquisitionType frame_type;
      int n_frames = 1000000000;

      /* check epoch */
      if (rnxEpoch() != workCommand.epoch) {
        DEBUG_LOG("RNX_WORK_STARTACQ ignored - epoch changed");
        continue;
      }

      if (pDetector && pFrameCallback) {
        sprintf(logbuf, "STARTACQ: _darkFlag=%s  _readoutMode=%d", _darkFlag ? "true" : "false", (int)_readoutMode);
        INFO_LOG(logbuf);
//      frame_type = _darkFlag ? ACQUIRE_DARK : ACQUIRE_DARK;
        frame_type = _darkFlag ? DarkFrameAcquisitionType : LightFrameAcquisitionType;

        error = pDetector->SetAcquisitionUserCB(pFrameCallback);
        if (error.IsError()) {
          sprintf(logbuf, "Error: SetAcquisitionUserCB() failed");
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "SetAcquisitionUserCB() done");
          printf("%s\n", logbuf);
          INFO_LOG(logbuf);
        }
        error = pDetector->SetupAcquisitionSequence(n_frames);
        if (error.IsError()) {
          sprintf(logbuf, "Error: SetupAcquisitionSequence(%d) failed", n_frames);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "SetupAcquisitionSequence(%d) done", n_frames);
          printf("%s\n", logbuf);
          INFO_LOG(logbuf);
        }
        error = pDetector->StartAcquisition(frame_type);
        if (error.IsError()) {
          sprintf(logbuf, "Thread #%d: Error: StartAcquisition() failed", tid);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "Thread #%d: StartAcquisition() done", tid);
          printf("%s\n", logbuf);
          INFO_LOG(logbuf);
        }
        error = pDetector->EndAcquisition();
        if (error.IsError()) {
          sprintf(logbuf, "Thread #%d: Error: EndAcquisition() failed", tid);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "Thread #%d: EndAcquisition() done", tid);
          printf("%s\n", logbuf);
          INFO_LOG(logbuf);
        }
      } else {
        ERROR_LOG("RNX_WORK_STARTACQ ignored - device not open");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_ENDACQ) {
      /* check epoch */
      if (rnxEpoch() != workCommand.epoch) {
        DEBUG_LOG("RNX_WORK_ENDACQ ignored - epoch changed");
        continue;
      }
      if (pDetector) {
        sprintf(logbuf, "Thread #%d Calling EndAcquisition(true) ...", tid);
        printf("%s\n", logbuf);
        INFO_LOG(logbuf);
        error = pDetector->EndAcquisition(true);
        if (error.IsError()) {
          sprintf(logbuf, "Thread #%d: Error: EndAcquisition(true) failed", tid);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "Thread #%d: EndAcquisition(true) done", tid);
          printf("%s\n", logbuf);
          INFO_LOG(logbuf);
        }
      } else {
        DEBUG_LOG("RNX_WORK_ENDACQ ignored - device not open");
      }
      continue;
    }

    printf("unrecognized command");
    ERROR_LOG("unrecognized command");
  }

  printf("Bye from workThread #%d!\n", tid);

  // remove?
  if (close(notifyFd) == -1) {
    perror("close");
  }
  pthread_exit(NULL);
}

using std::cout;
using std::cerr;
using std::endl;
using std::string;

RxDetector *openDetector(const char *configFileName, const char *detectorName)
{
  RxReturnStatus error;
  RxDetector *pDetector;

  /* RxDetector aDetector("config_foo"); */
  pDetector = new RxDetector(configFileName);
  if (!pDetector) {
    std::cerr << "new RxDetector is NULL" << std::endl;
    return (NULL);
  }

  // static VirtualFrameCallback* pFrameCallback = NULL;
  pFrameCallback = new FrameHandlerCB(*pDetector);
  if (!pFrameCallback) {
    std::cerr << "new FrameHandlerCB is NULL" << std::endl;
    return (NULL);
  }

  error = pDetector->Open(detectorName);
  if (error.IsError()) {
    std::cerr << "Open: " << error.ErrorText() << std::endl;
    return (NULL);
  }
  std::cout << "# Open()" << std::endl;

  return (pDetector);
}

int closeDetector(RxDetector *pDetector)
{
  int rv = -1;

  if (pDetector) {
    std::cout << "# Close()" << std::endl;
    pDetector->Close();
    rv = 0;
  } else {
    printf("%s: pDetector is NULL\n", __FUNCTION__);
  }
  return (rv);
}

int decodeConfigArgs(char *argstr, int *binning_f, int *binning_s, int *exposure, bool *rawFlag, bool *darkFlag, int *testPattern, int *readoutMode, int *triggerMode)
{
  char lilbuf[80];
  int rv = 0;
  int argc;
  char *saveptr;
  char *str1;
  char *xx;
  int rr;

  for (argc = 0, str1 = argstr; ; argc++, str1 = NULL) {
    xx = strtok_r(str1, ";\n\r ", &saveptr);
    if (!xx) {
      break;
    }
    // printf("%s: xx = '%s'\n", __FUNCTION__, xx);
    if ((strlen(xx) < 3) || !isalpha(xx[0]) || (xx[1] != '=')) {
      sprintf(lilbuf, "Config option '%s' not recognized", xx);
      ERROR_LOG(lilbuf);
      rv = -1;    /* ERROR */
      break;
    } else {
      switch (xx[0]) {
        case 'f':
          *binning_f = atoi(xx+2);
          break;
        case 's':
          *binning_s = atoi(xx+2);
          break;
        case 'e':
          *exposure = atoi(xx+2);
          break;
        case 'r':
          rr = atoi(xx+2);
          *rawFlag = (rr != 0);
          break;
        case 'd':
          rr = atoi(xx+2);
          *darkFlag = (rr != 0);
          break;
        case 'p':
          *testPattern = atoi(xx+2);
          break;
        case 'm':
          *readoutMode = atoi(xx+2);
          break;
        case 't':
          *triggerMode = atoi(xx+2);
          break;
        default:
          sprintf(lilbuf, "Config option '%c' not recognized", xx[0]);
          ERROR_LOG(lilbuf);
          rv = -1;    /* ERROR */
          break;
      }
    }
  }

  if (*binning_f != *binning_s) {
    sprintf(lilbuf, "Error: binning_f (%d) != binning_s (%d)", *binning_f, *binning_s);
    ERROR_LOG(lilbuf);
    rv = -1;    /* ERROR */
  }

  return (rv);
}

int setsndbuf(int socketFd, unsigned size)
{
  // NOP
  return 0;
}

int sendFrame(int frame_number, uint16_t timeStamp, uint16_t *frame)
{
  int ii, jj;
  data_footer_t *pFooter;
  int writeSize, sent;
  char logbuf[LOGBUF_SIZE];
  uint16_t frameNumber = (uint16_t)frame_number;
  uint16_t linebuf[4000];

  /* init footer */
  pFooter = (data_footer_t *)(linebuf + MAX_LINE_PIXELS);
  pFooter->cmd = 0;
  pFooter->frameNumber = frameNumber;
  pFooter->damage = 0;
  pFooter->epoch = _lastConfigEpoch;
  pFooter->binning_f = _binning_f;
  pFooter->binning_s = _binning_s;
  pFooter->pad = 0;

  /* send lines */
  sprintf(logbuf, "Send frame #%d lines...\n", frameNumber);
  // DEBUG_LOG(logbuf);
  INFO_LOG(logbuf);
  writeSize = (MAX_LINE_PIXELS * 2) + sizeof(data_footer_t);
  for (jj = 0; jj < MAX_LINE_PIXELS/_binning_s; jj += _binning_s) {

    /* copy pixels to output buffer */
    for (ii = 0; ii < MAX_LINE_PIXELS; ii++) {
      linebuf[ii] = *frame++;
    }

    /* first 2 pixels of frame hold frame # and timestamp */
    if (jj == 0) {
      linebuf[0] = frameNumber;
      linebuf[1] = timeStamp;
    }

    /* update data footer */
    pFooter->lineNumber = jj;

    /* write to data socket */
    if (frameNumber & 1) {
      // odd frame -> odd data port
      sent = sendto(dataFdOdd, (void *)linebuf, writeSize, 0,
                 (struct sockaddr *)&dataaddrodd, sizeof(dataaddrodd));
    } else {
      // even frame -> even data port
      sent = sendto(dataFdEven, (void *)linebuf, writeSize, 0,
                 (struct sockaddr *)&dataaddreven, sizeof(dataaddreven));
    }
    if (sent == -1) {
      perror("sendto");
      sprintf(logbuf, "Frame #%d sendto() failed (errno=%d)",
              frameNumber, errno);
      printf("%s\n", logbuf);
      ERROR_LOG(logbuf);
      break;
    } else if (sent != writeSize) {
      sprintf(logbuf, "Error: Sent %d of %d bytes for Frame #%d Line #%d",
              sent, writeSize, frameNumber, jj);
      printf("%s\n", logbuf);
      ERROR_LOG(logbuf);
    }
  }

  /* write to notification socket */
  sprintf(logbuf, "Send frame #%d notification...", frameNumber);
  DEBUG_LOG(logbuf);
  sent = sendto(notifyFd, (void *)pFooter, sizeof(data_footer_t), 0,
             (struct sockaddr *)&notifyaddr, sizeof(notifyaddr));
  if (sent == -1) {
    perror("sendto");
    ERROR_LOG("Failed sendto() on notify socket");
  } else if (sent != sizeof(data_footer_t)) {
    printf("sent %d of %lu bytes\n", sent, sizeof(data_footer_t));
    ERROR_LOG("sendto() notify socket returned incorrect size");
  }
  return (0);
}
