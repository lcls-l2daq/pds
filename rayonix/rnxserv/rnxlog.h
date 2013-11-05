/* rnxlog.h */
#ifndef _RNX_LOG_H
#define _RNX_LOG_H

#define SYSLOG_PORT   514
#define SYSLOG_IPADDR "10.0.1.1"
#define LOGBUF_SIZE   256

#define FACILITY_USER 1

#define LEVEL_ERROR   3
#define LEVEL_WARNING 4
#define LEVEL_NOTICE  5
#define LEVEL_INFO    6
#define LEVEL_DEBUG   7

#define ERROR_LOG(x)    rnxlog(LEVEL_ERROR, __FILE__, __LINE__, x)
#define WARNING_LOG(x)  rnxlog(LEVEL_WARNING, __FILE__, __LINE__, x)
#define NOTICE_LOG(x)   rnxlog(LEVEL_NOTICE, __FILE__, __LINE__, x)
#define INFO_LOG(x)     rnxlog(LEVEL_INFO, __FILE__, __LINE__, x)
#define DEBUG_LOG(x)    rnxlog(LEVEL_DEBUG, __FILE__, __LINE__, x)

int rnxlogInit(int level, const char *ident);
int rnxlog(int level, const char *file, int line, const char *msg);

#endif /* _RNX_LOG_H */
