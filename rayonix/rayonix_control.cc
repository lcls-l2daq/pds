// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "rayonix_control.hh"

// forward declarations
static int open_socket(const char *host, int port, bool verbose);
ssize_t SendCommand(int fd, const char *cmd, char *reply, size_t replySize);
ssize_t Readline(int fd, void *vptr, size_t maxlen);
ssize_t Writeline(int fc, const void *vptr, size_t maxlen);

using namespace Pds;

rayonix_control::rayonix_control(bool verbose, const char *host, int port) :
  _verbose(verbose),
  _port(port),
  _connected(false),
  _control_fd(-1)
{
  strncpy(_host, host, HostnameMax);
  if (_verbose) {
    printf("%s: _port = %d\n", __FUNCTION__, _port);
  }
}

rayonix_control::~rayonix_control()
{
}

/*
 * connect -
 *
 * RETURNS: -1 on error, otherwise 0.
 */
int rayonix_control::connect()
{
  int rv = -1;
  int fd = -1;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  // open socket
  fd = open_socket(_host, _port, _verbose);
  if (fd > 0) {
    char lilbuf[256];
    if (_verbose) {
      printf("%s: open_socket() successful\n", __FUNCTION__);
    }
    // read server greeting
    lilbuf[0] = '\0';
    Readline(fd, lilbuf, sizeof(lilbuf)-1);
    if (_verbose && lilbuf[0]) {
      printf("Server greeting: %s\n", lilbuf);
    }
    // send "hello"
    SendCommand(fd, "hello\n", lilbuf, sizeof(lilbuf));
    if (_verbose && lilbuf[0]) {
      printf("Reply to hello: %s\n", lilbuf);
    }
    // FIXME check for valid reply

    _control_fd = fd;
    _connected = true;
    rv = 0;   /* OK */
  }

  return (rv);
}

int rayonix_control::config(int binning_f, int binning_s, int exposure, int rawMode,
                            int readoutMode, int trigger, int testPattern, uint8_t darkFlag,
                            char *deviceID)
{
  int rv = -1;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }
  if (_connected && (_control_fd > 0)) {
    char    sendbuf[80];
    char    replybuf[256];

    sprintf(sendbuf, "conf f=%d;s=%d;e=%d;r=%d;m=%d;t=%d;p=%d;d=%d\n", binning_f, binning_s, exposure, rawMode,
            readoutMode, trigger, testPattern, (int)darkFlag);
    SendCommand(_control_fd, sendbuf, replybuf, sizeof(replybuf));
    if (_verbose && replybuf[0]) {
      printf("Reply to '%s': '%s'\n", sendbuf, replybuf);
    }
    if (replybuf[0] == '2') {
      char *pos1, *pos2;
      rv = 0; // Success
      if (deviceID) {
        pos1 = strstr(replybuf, "deviceID=");
        if (pos1) {
          // find string between '=' and ' '
          pos1 = index(pos1, '=') + 1;
          // null terminate at space, if found
          pos2 = index(pos1, ' ');
          if (pos2) {
            *pos2 = '\0';
          }
          // null terminate at semicolon, if found
          pos2 = index(pos1, ';');
          if (pos2) {
            *pos2 = '\0';
          }
          snprintf(deviceID, DeviceIDMax, "%s", pos1);
        } else {
          snprintf(deviceID, DeviceIDMax, "Unknown");
        }
      }
    }
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::calib()
{
  int rv = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_connected && (_control_fd > 0)) {
    if (_verbose) {
      printf("     %s not implemented\n", __FUNCTION__);
    }
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::enable(void)
{
  int rv = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_connected && (_control_fd > 0)) {
    char lilbuf[256];

    // send "enab"
    Writeline(_control_fd, "enab\n", 5);

    // read reply
    lilbuf[0] = '\0';
    Readline(_control_fd, lilbuf, sizeof(lilbuf)-1);
    if (_verbose && lilbuf[0]) {
      printf("Reply to enab: %s\n", lilbuf);
    }
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::disable(void)
{
  int rv = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_connected && (_control_fd > 0)) {
    char lilbuf[256];

    // send "disa"
    SendCommand(_control_fd, "disa\n", lilbuf, sizeof(lilbuf));
    if (_verbose && lilbuf[0]) {
      printf("Reply to disa: %s\n", lilbuf);
    }
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::reset(void)
{
  int rv = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_connected && (_control_fd > 0)) {
    char lilbuf[256];

    if (_verbose) {
      printf("%s: sending reset\n", __FUNCTION__);
    }

    // send "reset"
    SendCommand(_control_fd, "reset\n", lilbuf, sizeof(lilbuf));
    if (_verbose && lilbuf[0]) {
      printf("Reply to reset: %s\n", lilbuf);
    }
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::disconnect(void)
{
  int rv = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }
  if (_connected && (_control_fd > 0)) {
    char lilbuf[256];

    if (_verbose) {
      printf("%s: sending quit\n", __FUNCTION__);
    }

    // send "quit"
    SendCommand(_control_fd, "quit\n", lilbuf, sizeof(lilbuf));
    if (_verbose && lilbuf[0]) {
      printf("Reply to quit: %s\n", lilbuf);
    }

    // close socket
    if (close(_control_fd) == -1) {
      perror("close");
    } else if (_verbose) {
      printf("%s: Control socket closed\n", __FUNCTION__);
    }
    _connected = false;
    _control_fd = -1;
  } else {
    printf("Error: called %s while unconnected\n", __FUNCTION__);
  }

  return (rv);
}

int rayonix_control::status(void)
{
  int rv = Unconnected;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }
  if (_connected && (_control_fd > 0)) {
    char lilbuf[256];

    rv = -1;    // invalid

    // send "status"
    Writeline(_control_fd, "stat\n", 5);

    // read reply
    lilbuf[0] = '\0';
    Readline(_control_fd, lilbuf, sizeof(lilbuf)-1);
    if (lilbuf[0]) {
      if (_verbose && lilbuf[0]) {
        printf("Reply to stat: %s\n", lilbuf);
      }
      if ((lilbuf[0] == '2') && (strlen(lilbuf) >= 3)) {
        switch(lilbuf[2]) {
          case '1':
            rv = Unconfigured;
            break;
          case '2':
            rv = Configured;
            break;
          case '3':
            rv = Enabled;
            break;
          default:
            printf("%s: Error - unknown status value '%c'\n", __FUNCTION__, lilbuf[2]);
            break;
        }
      }
    }
  }

  return (rv);
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 500

static int open_socket(const char *host, int port, bool verbose)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int flag = 1; 
    char portbuf[10];

    if (verbose) {
      printf("entered %s(\"%s\", %d, %s)\n", __FUNCTION__, host, port, verbose ? "true" : "false");
    }

   /* Obtain address(es) matching host/port */

   memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    sprintf(portbuf, "%d", port);
    if (verbose) {
      printf("%s: host=\"%s\" portbuf=\"%s\"\n", __FUNCTION__, host, portbuf); 
    }
    s = getaddrinfo(host, portbuf, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return (-1);  /* ERROR */
    }

   /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

   for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

       if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

       close(sfd);
    }

   if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return (-1);  /* ERROR */
    }

   freeaddrinfo(result);           /* No longer needed */

  /* disable Nagle algorithm */
  setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

  return (sfd);
}

ssize_t SendCommand(int fd, const char *cmd, char *reply, size_t replySize)
{
  int rv;
  Writeline(fd, cmd, strlen(cmd));

  // read reply
  *reply = '\0';
  rv = Readline(fd, reply, replySize-1);

  return (rv);
}

/*

  HELPER.C
  ========
  (c) Paul Griffiths, 1999
  Email: mail@paulgriffiths.net

  Implementation of sockets helper functions.

  Many of these functions are adapted from, inspired by, or 
  otherwise shamelessly plagiarised from "Unix Network 
  Programming", W Richard Stevens (Prentice Hall).

*/

/*  Read a line from a socket  */

ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = (char *)vptr;

    for ( n = 1; (int)n < (int)maxlen; n++ ) {
    
    if ( (rc = read(sockd, &c, 1)) == 1 ) {
        *buffer++ = c;
        if ( c == '\n' )
        break;
    }
    else if ( rc == 0 ) {
        if ( n == 1 )
        return 0;
        else
        break;
    }
    else {
        if ( errno == EINTR )
        continue;
        return -1;
    }
    }

    *buffer = 0;
    return n;
}


/*  Write a line to a socket  */

ssize_t Writeline(int sockd, const void *vptr, size_t n) {
  size_t      nleft;
  ssize_t     nwritten;
  const char *buffer;

  buffer = (const char *)vptr;
  nleft  = n;

  while ( nleft > 0 ) {
    if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
      if ( errno == EINTR )
        nwritten = 0;
      else
        return -1;
    }
    nleft  -= nwritten;
    buffer += nwritten;
  }

  return n;
}
