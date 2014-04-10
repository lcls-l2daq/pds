/* workThread.h */

#ifndef _WORK_THREAD_H
#define _WORK_THREAD_H

#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>

#define NUM_WORK_THREADS  2

#define WORK_ARGV_MAXLEN  243

#define DETECTOR_NAME_HW  ""
// this will work also #define DETECTOR_NAME_HW  "Detector"
#define DETECTOR_NAME_SIM "MX170HS-swsim"

// #define CONFIG_FILE_NAME  "RxDetector.conf"
// use default
#define CONFIG_FILE_NAME  ""

/* workCmd_t is used for both command and reply */
typedef struct {
  int cmd;
  unsigned int epoch;
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  char argv[WORK_ARGV_MAXLEN+1];
} workCmd_t;

typedef struct {
  int task_id;
  int work_pipe_fd;     /* input */
  int control_pipe_fd;  /* output */
  sem_t *pConfigSem;    /* config and calib semaphore */
  bool simFlag;
} work_state_t;

typedef struct {
  uint16_t  frameNumber;
  uint16_t  lineNumber;
  uint16_t  flags;
  uint16_t  pad;
} line_header_t;

void *workRoutine(void *arg);

#endif /* _WORK_THREAD_H */
