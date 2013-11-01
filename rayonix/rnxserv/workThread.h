/* workThread.h */

#ifndef _WORK_THREAD_H
#define _WORK_THREAD_H

#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>

#define NUM_WORK_THREADS  2

#define WORK_ARGV_MAXLEN  243
#define DETECTOR_NAME  "MX170HS"
// #define DETECTOR_NAME     "Sim-MX170HS"
#define CONFIG_FILE_NAME  "RxDetector.conf"

/* workCmd_t is used for both command and reply */
typedef struct {
  int cmd;
  unsigned int epoch;
  int arg1;
  char argv[WORK_ARGV_MAXLEN+1];
} workCmd_t;

typedef struct {
  int task_id;
  int work_pipe_fd;     /* input */
  int control_pipe_fd;  /* output */
  sem_t *pConfigSem;    /* config and calib semaphore */
} work_state_t;

typedef struct {
  uint16_t  frameNumber;
  uint16_t  lineNumber;
  uint16_t  flags;
  uint16_t  pad;
} line_header_t;

void *workRoutine(void *arg);

#endif /* _WORK_THREAD_H */
