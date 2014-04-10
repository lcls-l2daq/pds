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

#include "craydl/craydl.h"
// #include "RxDetector.h"

#define DEST_IPADDR         "10.0.1.1"
#define NOTIFY_PORT         30050

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
static craydl::ReadoutMode_t  _readoutMode = craydl::ReadoutModeStandard;
static bool _verbose = false;    // FIXME command line parameter
static bool _error = false;

// forward declaration
int sendFrame(int frameNumber, uint16_t timeStamp, uint16_t *frame);

static void die(int tid)
{
  printf("Bye from workThread #%d!\n", tid);
  sleep (1);
  pthread_exit(NULL);
}

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
         frame_number_width_(4),
         filename_(""),
         filename_root_("dummy."),
         raw_filename_root_("dummy_raw.")
      {
      }
      ~FrameHandlerCB() {}
      void SequenceStarted() {RxLog(LOG_DEBUG) << "CB: Sequence Started ()" << std::endl;}
      void SequenceEnded() {RxLog(LOG_DEBUG) << "CB: Sequence Ended ()" << std::endl;}    // look for this one
      void ExposureStarted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Exposure " << frame_number << " started." << std::endl;}
      void ExposureEnded(int frame_number) {RxLog(LOG_DEBUG) << "CB: Exposure " << frame_number << " ended." << std::endl;}
      void ReadoutStarted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Readout " << frame_number << " started." << std::endl;}
      void ReadoutEnded(int frame_number) {RxLog(LOG_DEBUG) << "CB: Readout " << frame_number << " ended." << std::endl;}
      void BackgroundFrameReady(const RxFrame *frame_p) { RxLog(LOG_DEBUG) << "CB: Background Frame " << " buffer address: " << frame_p->getBufferAddress() << std::endl; }
#if 0
      void RawFrameReady(int frame_number, const RxFrame *frame_p)
      { 
         RxLog(LOG_VERBOSE) << "CB: Raw Frame " << frame_number << " ready. (" << frame_p->getNFast() << "x" <<frame_p->getNSlow() << "x" << frame_p->getDepth() << ") at " <<    frame_p->getBufferAddress() << ")" << std::endl;
         RxLog(LOG_VERBOSE) << "CB: Raw Frame " << frame_number << " Timestamp: " << frame_p->constMetaData().HardwareTimestamp() << std::endl;

         return;
      }

      void FrameReady(int frame_number, const RxFrame *frame_p)
      {

         RxLog(LOG_VERBOSE) << "CB: Frame " << frame_number << " ready. (" << frame_p->getNFast() << "x" <<frame_p->getNSlow() << "x" << frame_p->getDepth() << ") at " <<    frame_p->getBufferAddress() << ")" << std::endl;
         RxLog(LOG_VERBOSE) << "CB: Frame " << frame_number << " size is " << frame_p->getSize() << std::endl;
         RxLog(LOG_VERBOSE) << "CB: Frame " << frame_number << " Timestamp: " << frame_p->constMetaData().HardwareTimestamp() << std::endl;

         return;
      }
#else

      void RawFrameReady(int frame_number, const RxFrame *frame_p)
      {
         if (_rawFlag) {
           boost::posix_time::ptime ppp = frame_p->constMetaData().HardwareTimestamp();
           boost::posix_time::time_duration duration( ppp.time_of_day() );
           unsigned short timeStamp = duration.total_milliseconds() % 0xffff;

           if (_verbose) {
             printf("%s: Raw Frame #%d  size=%u  timestamp=%hu ms\n", __FUNCTION__, frame_number, (unsigned)frame_p->getSize(), timeStamp);
           }

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

#endif
      void FrameAborted(int frame_number) {RxLog(LOG_WARNING) << "CB: Frame " << frame_number << " aborted." << std::endl;}
      void FrameCompleted(int frame_number) {RxLog(LOG_DEBUG) << "CB: Frame " << frame_number << " completed." << std::endl;}
      void FrameError(int frame_number, const int error_code, const std::string& error_string) {RxLog(LOG_ERROR) << "CB: Frame " << frame_number << " reported error " << error_code << ":" << error_string << std::endl;}

      void set_write_frames(const bool write_frames = true) {
      }

      void set_filename(const std::string &filename) {
      }
      void set_filename_sequence(bool multi_sequence, int sequence_number = 0, int width = 2, std::string seperator = "_")
      {
      }
      void set_filename_generator(const std::string& raw_root, const std::string& root, int width, const std::string& suffix)
      {
      }
      void set_filename_generator(const std::string& root, int width, const std::string& suffix)
      {
      }
      std::string get_background_filename()
      {
         return string("");
      }
      std::string get_raw_filename(int frame_number)
      {
         return string("");
      }

      std::string get_filename(int frame_number)
      {
         return string("");
      }

   private:

      RxDetector& detector_;
      bool multi_sequence_;
      int sequence_number_;
      int sequence_number_width_;
      std::string sequence_seperator_;
      bool write_frames;
      int frame_number_width_;
      std::string filename_;
      std::string filename_root_;
      std::string filename_suffix_;
      std::string raw_filename_;
      std::string raw_filename_root_;
      std::string raw_filename_suffix_;
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

int sendWorkCommand(work_state_t *pState, workCmd_t *pCommand)
{
  if (write(pState->control_pipe_fd, pCommand, sizeof(workCmd_t)) == -1) {
    perror("write");
    ERROR_LOG("error writing to control pipe");
    return (-1);    /* ERROR */
  }
  return (0);       /* OK */
}

void *workRoutine(void *arg)
{
  work_state_t *myState = (work_state_t *)arg;
  int tid = myState->task_id;
  bool simFlag = myState->simFlag;
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

  /* open device once, then leave it open */
  if (sem_wait(myState->pConfigSem) == -1) {          /* semaphore take */
    perror("sem_wait pConfigSem");
    ERROR_LOG("sem_wait() failed");
    die(tid);
  } else {
    /* check whether another thread already opened the device */
    if (!pDetector) {
      /* open detector... */
      pDetector = openDetector(CONFIG_FILE_NAME, simFlag ? DETECTOR_NAME_SIM : DETECTOR_NAME_HW);
      if (pDetector) {
        /* ...open succeeded */
        snprintf(logbuf, sizeof(logbuf), "%s: Opened device %s", __FUNCTION__, simFlag ? DETECTOR_NAME_SIM : DETECTOR_NAME_HW);
        printf("%s\n", logbuf);
        INFO_LOG(logbuf);
        craydl::Log::IncrementReportingLevel(1);
        if (_verbose) {
          craydl::Log::IncrementReportingLevel(1);
          printf("** verbose **\n");
        }
      } else {
        /* ...open failed: fatal error */
        snprintf(logbuf, sizeof(logbuf), "%s: openDetector(%s, %s) returned NULL", __FUNCTION__,
                 CONFIG_FILE_NAME, simFlag ? DETECTOR_NAME_SIM : DETECTOR_NAME_HW);
        printf("%s\n", logbuf);
        ERROR_LOG(logbuf);
        printf("Try, as root\n\n"
               "   # lsof | grep ttyS0\n\n"
               "To see if another process has the device open\n");
        die(tid);
      }
    }
    if (sem_post(myState->pConfigSem) == -1) {        /* semaphore give */
      perror("sem_post pConfigSem");
      ERROR_LOG("sem_post() failed");
    }
  }

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
        case  RNX_WORK_SWTRIGGER:
          sprintf(logbuf2, "Thread #%d: SWTRIGGER\n", tid);
          break;
        default:
          sprintf(logbuf2, "Thread #%d: cmd=%d arg1=%d arg2=%d arg3=%d arg4=%d epoch=%u\n", tid,
                  workCommand.cmd, workCommand.arg1, workCommand.arg2, workCommand.arg3, workCommand.arg4, workCommand.epoch);
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
      int testPattern = 0;
      int readoutMode = craydl::ReadoutModeStandard;
      int triggerMode = 0;      // FIXME default trigger mode (0=hw, 1=sw)
      int fast, slow, depth;
      int errorCount = 0;
      if (_verbose) {
        printf(" ** workCommand.cmd == RNX_WORK_OPENDEV **\n");
      }
      /* interpret arguments */
      if (decodeConfigArgs(workCommand.argv, &binning_f, &binning_s, &exposure_ms, &rawFlag, &darkFlag, &testPattern, &readoutMode, &triggerMode) == -1) {
        snprintf(logbuf, sizeof(logbuf), "%s: decodeConfigArgs() returned -1", __FUNCTION__);
        printf("%s\n", logbuf);
        ERROR_LOG(logbuf);
        workReply.cmd = 5;  /* error */
#if 0
        if (write(myState->control_pipe_fd, &workReply, sizeof(workReply)) == -1) {
          perror("write");
          ERROR_LOG("error writing to control pipe");
        }
#else
        sendWorkCommand(myState, &workReply);
#endif
        continue;   /* did not take semaphore */
      }
      /* semaphore take */
      if (sem_wait(myState->pConfigSem) == -1) {
        perror("sem_wait pConfigSem");
        ERROR_LOG("sem_wait() failed");
      } else if (rnxEpoch() != workCommand.epoch) {
        ERROR_LOG("epoch changed before configuration - skipping config");
      } else if (rnxEpoch() == _lastConfigEpoch) {
        ERROR_LOG("duplicate config for single epoch - skipping config");
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
          error = pDetector->SetBinning(binning_f); /* TODO support binning_s */ /* FIXME set binning later too */
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

          /* set interval */
          error = pDetector->SetIntervalTime((double)0.0);
          if (error.IsError()) {
            sprintf(logbuf, "Error: SetIntervalTime(0.0) failed");
            printf("%s\n", logbuf);
            ERROR_LOG(logbuf);
            errorCount++;
          } else {
            sprintf(logbuf, "SetIntervalTime(0.0) done");
            printf("%s\n", logbuf);
            INFO_LOG(logbuf);
          }

          // Check - will print to log file
          // ? pDetector->TriggerType();

          /* set modes */
          _rawFlag = rawFlag;
          _darkFlag = darkFlag;
          switch (readoutMode) {
            case ReadoutModeHighGain:
              _readoutMode = ReadoutModeHighGain;
              break;
            case ReadoutModeHDR:
              _readoutMode = ReadoutModeHDR;
              break;
            default:
              sprintf(logbuf, "Warning: Readout mode %d not supported", (int) readoutMode);
              printf("%s\n", logbuf);
              WARNING_LOG(logbuf);
              /* FIXME sendUserMessage(logbuf); */
              /* fall through */
            case ReadoutModeStandard:
              _readoutMode = ReadoutModeStandard;
              break;
          }
          error = pDetector->SetReadoutMode(craydl::ReadoutMode(_readoutMode));
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

          pDetector->SetFrameTriggerMode(FrameTriggerType(FrameTriggerTypeFrame));
          pDetector->SetSequenceGate("None");
          pDetector->SetSequenceGateSignalType("None");
          switch (triggerMode) {
            // Important for frame transfer mode: exposure and interval time both set to 0.0
            case 1:
              // software trigger
              INFO_LOG("Trigger Mode 1: software");
              pDetector->SetFrameTriggerSignalType(DigitalIOSignalType(DigitalIOSignalTypeSoftware));
              break;
            default:
              sprintf(logbuf, "Warning: Trigger mode %d not supported", triggerMode);
              printf("%s\n", logbuf);
              WARNING_LOG(logbuf);
              /* fall through */
            case 0:
              // hardware trigger
              INFO_LOG("Trigger Mode 0: hardware");
              pDetector->SetFrameTriggerSignalType(DigitalIOSignalType(DigitalIOSignalTypeOpto));
              break;
          }
          error = pDetector->SetReadoutMode(craydl::ReadoutMode(_readoutMode));
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
            error = pDetector->SetTestPattern(craydl::TestPattern((TestPattern_t)testPattern));
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
              error = pDetector->EnableTestPattern(true);
              if (error.IsError()) {
                sprintf(logbuf, "EnableTestPattern(true) failed");
                printf("%s\n", logbuf);
                ERROR_LOG(logbuf);
                errorCount++;
              } else {
                sprintf(logbuf, "EnableTestPattern(true) done");
                printf("%s\n", logbuf);
                INFO_LOG(logbuf);
              }
            }
          } else {
            error = pDetector->EnableTestPattern(false);
            if (error.IsError()) {
              sprintf(logbuf, "EnableTestPattern(false) failed");
              printf("%s\n", logbuf);
              ERROR_LOG(logbuf);
              errorCount++;
            } else {
              _testPattern = 0;
              sprintf(logbuf, "EnableTestPattern(false) done");
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
        workReply.arg2 = (int) _readoutMode;
        workReply.arg3 = _testPattern;
        workReply.arg4 = 0;

        printf("%swork%d task sending cmd <%d> framesize <%d> readoutMode <%d> testPattern <%d> epoch <%u> detectorID <%s>\n",
               myState->task_id ? "  " : "", myState->task_id, workReply.cmd,
               workReply.arg1, workReply.arg2, workReply.arg3, workReply.epoch, workReply.argv);
#if 0
        if (write(myState->control_pipe_fd, &workReply, sizeof(workReply)) == -1) {
          perror("write");
          ERROR_LOG("error writing to control pipe");
        }
#else
        sendWorkCommand(myState, &workReply);
#endif
      }
      /* semaphore give */
      if (sem_post(myState->pConfigSem) == -1) {
        perror("sem_post pConfigSem");
        ERROR_LOG("sem_post() failed");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_CLOSEDEV) {     /* DO NOT USE */
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
      if (_error) {
        /* error injection */
        if (notifyMsg.frameNumber == 666) {
          notifyMsg.frameNumber = frameCountAdvance();
        }
      }
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
      if (_verbose) {
        sprintf(logbuf, "Thread #%d: Send frame #%d lines...", tid, notifyMsg.frameNumber);
        printf("%s\n", logbuf);  /* verbose */
        DEBUG_LOG(logbuf);
      }
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
      if (_verbose) {
        sprintf(logbuf, "Thread #%d: Send frame #%d notification...", tid, notifyMsg.frameNumber);
        printf("%s\n", logbuf);  /* verbose */
        DEBUG_LOG(logbuf);
      }
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
      int n_frames = 0;   // 0=infinite
      // RxFrameAcquisitionType frame_type;
      FrameAcquisitionType frame_type;

      /* check epoch */
      if (rnxEpoch() != workCommand.epoch) {
        DEBUG_LOG("RNX_WORK_STARTACQ ignored - epoch changed");
        continue;
      }
      /* semaphore take */
      if (sem_wait(myState->pConfigSem) == -1) {
        perror("sem_wait pConfigSem");
        ERROR_LOG("sem_wait() failed");
      }

      if (pDetector) {
        sprintf(logbuf, "STARTACQ: _darkFlag=%s _readoutMode=%d _testPattern=%d", _darkFlag ? "true" : "false",
                (int)_readoutMode, _testPattern);
        INFO_LOG(logbuf);

//  FIXME set binning to reflect configuration, not only at device open
//      error = pDetector->SetBinning(binning_f);

        frame_type = _darkFlag ? FrameAcquisitionTypeDark : FrameAcquisitionTypeLight;
        error = pDetector->SetAcquisitionUserCB(pFrameCallback);    // only needed once
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

      /* semaphore give */
      if (sem_post(myState->pConfigSem) == -1) {
        perror("sem_post pConfigSem");
        ERROR_LOG("sem_post() failed");
      }
      continue;
    }

    if (workCommand.cmd == RNX_WORK_ENDACQ) {
      printf("** RNX_WORK_ENDACQ **\n");
      /* check epoch */
      if (rnxEpoch() != workCommand.epoch) {
        DEBUG_LOG("RNX_WORK_ENDACQ ignored - epoch changed");
        continue;
      }
#if 0
      /* semaphore take */
      if (sem_wait(myState->pConfigSem) == -1) {
        perror("sem_wait pConfigSem");
        ERROR_LOG("sem_wait() failed");
      }
#endif
      if (pDetector) {
        error = pDetector->EndAcquisition(true);    // abort=true
        if (error.IsError()) {
          sprintf(logbuf, "Error: EndAcquisition(true) failed");
          ERROR_LOG(logbuf);
        } else {
          sprintf(logbuf, "EndAcquisition(true) succeeded");
          INFO_LOG(logbuf);
        }
        printf("%s\n", logbuf);
      } else {
        DEBUG_LOG("RNX_WORK_ENDACQ ignored - device not open");
      }
#if 0
      /* semaphore give */
      if (sem_post(myState->pConfigSem) == -1) {
        perror("sem_post pConfigSem");
        ERROR_LOG("sem_post() failed");
      }
#endif
      continue;
    }

    if (workCommand.cmd == RNX_WORK_SWTRIGGER) {

      if (pDetector) {

        INFO_LOG("SWTRIGGER");

        double pulseArg = 0.01;
        error = pDetector->PulseBulb(pulseArg);
        if (error.IsError()) {
          sprintf(logbuf, "Error: PulseBulb(%5.5g) failed", pulseArg);
          printf("%s\n", logbuf);
          ERROR_LOG(logbuf);
        }
      } else {
        ERROR_LOG("RNX_WORK_SWTRIGGER ignored - device not open");
      }

      continue;
    }

    printf("unrecognized command");
    ERROR_LOG("unrecognized command");
  }

  if (close(notifyFd) == -1) {
    perror("close");
  }
  die(tid);
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
