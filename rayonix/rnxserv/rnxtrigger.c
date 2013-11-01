/* rnxtrigger.c */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rnxtrigger.h"

int                 _udp_socket = 0;
struct sockaddr_in  _to_addr;
int                 _initDone = 0;

int rnxtriggerInit(int level)
{
  if (!_initDone) {
    _to_addr.sin_family      = AF_INET;
    _to_addr.sin_port        = TRIGGER_PORT;
    _to_addr.sin_addr.s_addr = inet_addr(TRIGGER_IPADDR);

    _udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (_udp_socket == -1) {
      perror("socket");
      _udp_socket = 0;
    } else {
      _initDone = 1;
    }
  }
  return (0);
}

int rnxtrigger(int cmd)
{
  int           rv = -1;
  int           bytes_sent;

  if (!_initDone) {
    printf("not initialized\n");
    return (-1);
  }

  bytes_sent = sendto(_udp_socket, (char *)&cmd, sizeof(cmd), 0,
                      (struct sockaddr *)&_to_addr, sizeof(_to_addr));

  if (bytes_sent == -1) {
    perror("sendto");
  } else {
    rv = 0;   /* return OK */
  }

  return (rv);
}
