/* rnxcommon.h */
#ifndef _RNX_COMMON_H
#define _RNX_COMMON_H

int frameCountAdvance();
int frameCountReset();

#define RNX_CONTROL_PORT      30042

#define RNX_WORK_FRAMEREADY   10

#define RNX_WORK_OPENDEV      20
#define RNX_WORK_CLOSEDEV     21
#define RNX_WORK_STARTACQ     22
#define RNX_WORK_ENDACQ       23
#define RNX_WORK_SWTRIGGER    24
#define RNX_WORK_DARK         25

#define RNX_BINNING_MIN       2
#define RNX_BINNING_MAX       10

#endif /* _RNX_COMMON_H */
