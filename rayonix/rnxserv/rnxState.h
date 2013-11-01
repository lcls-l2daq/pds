#ifndef _RNX_STATE_H
#define _RNX_STATE_H

enum { RNX_STATE_UNCONFIGURED=1, RNX_STATE_CONFIGURED=2, RNX_STATE_ENABLED=3 }; 

int rnxState();
unsigned int rnxEpoch();
int rnxLogLevel();

#endif /* _RNX_STATE_H */
