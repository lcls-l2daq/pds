/* rnxlog.c */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rnxlog.h"
#include <syslog.h>

int                 _udp_socket = 0;
struct sockaddr_in  _to_addr;
int                 _initDone = 0;
int                 _reportLevel = LEVEL_DEBUG;

int rnxlogInit(int level, const char *ident)
{
  if (!_initDone) {
    openlog(ident, 0, LOG_USER);
    _reportLevel = level;
    _initDone = 1;
  }
  return (0);
}

int rnxlog(int level, const char *file, int line, const char *msg)
{
  if (level > _reportLevel) {
    return (0); /* skip */
  }

  if (!_initDone) {
    printf("not initialized\n");
    return (-1);
  }

  syslog(level, "%s:%d %s", file, line, msg);
  return (0);
}
