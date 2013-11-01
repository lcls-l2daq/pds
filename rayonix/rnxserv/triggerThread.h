/* triggerThread.h */

#ifndef _TRIGGER_THREAD_H
#define _TRIGGER_THREAD_H

#include "rnxState.h"

#define RNX_TRIGGER_MODE_TIMER  0
#define RNX_TRIGGER_MODE_UDP    1

typedef struct {
  int cmd;
} trigger_cmd_t;

typedef struct {
  int task_id;
  int mode;       /* 0=timer, 1=UDP port 30051 */
  int read_socket;
  int write_pipe_fd;
} trigger_state_t;

void *triggerRoutine(void *arg);

#endif /* _TRIGGER_THREAD_H */
