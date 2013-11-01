/* rnxtrigger.h */
#ifndef _RNX_TRIGGER_H
#define _RNX_TRIGGER_H

#define TRIGGER_PORT   30051
#define TRIGGER_IPADDR "10.0.1.101"

int rnxtriggerInit(int level);
int rnxtrigger(int cmd);

#endif /* _RNX_TRIGGER_H */
