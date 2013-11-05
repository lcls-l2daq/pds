// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "rayonix_data.hh"

#define ROUND_UP(x,y)       (((x)+(y)-1)/(y)*(y))

// forward declarations
int createUdpSocket(int port);
int setrcvbuf(int socketFd, unsigned size);

using namespace Pds;

Pds::rayonix_data::rayonix_data(unsigned bufsize, bool verbose) :
  _bufsize(bufsize)
{
  // allocate buffers
  _discard = new char[_bufsize];  // buffer for draining input buffers

  // create notify socket
  _notifyFd = createUdpSocket(RNX_NOTIFY_PORT);
  if (verbose) {
    if (_notifyFd) {
      printf("   createUdpSocket(%d) returned %d\n\r", RNX_NOTIFY_PORT, _notifyFd);
    } else {
      printf("Error: createUdpSocket(%d) returned 0\n\r", RNX_NOTIFY_PORT);
    }
  }

  // create even data socket
  _dataFdEven = createUdpSocket(RNX_DATA_PORT_EVEN);
  if (verbose) {
    if (_dataFdEven) {
      printf("   createUdpSocket(%d) returned %d\n\r", RNX_DATA_PORT_EVEN, _dataFdEven);
    } else {
      printf("Error: createUdpSocket(%d) returned 0\n\r", RNX_DATA_PORT_EVEN);
    }
  }

  // create odd data socket
  _dataFdOdd = createUdpSocket(RNX_DATA_PORT_ODD);
  if (verbose) {
    if (_dataFdOdd) {
      printf("   createUdpSocket(%d) returned %d\n\r", RNX_DATA_PORT_ODD, _dataFdOdd);
    } else {
      printf("Error: createUdpSocket(%d) returned 0\n\r", RNX_DATA_PORT_ODD);
    }
  }
}

Pds::rayonix_data::~rayonix_data()
{
  if (_notifyFd) {
    close(_notifyFd);
  }
  if (_dataFdEven) {
    close(_dataFdEven);
  }
  if (_dataFdOdd) {
    close(_dataFdOdd);
  }
}

int Pds::rayonix_data::drainFd(int fd) const
{
  int rv;

  while ((rv = recvfrom(fd, _discard, _bufsize, MSG_DONTWAIT, 0, 0)) > 0) {
    ;
  }
  return (rv);
}

int Pds::rayonix_data::reset(bool verbose) const
{
  int rv = -1;  /* ERROR */

  if ((_notifyFd > 0) && (_dataFdEven > 0) && (_dataFdOdd > 0)) {
    // drain input buffers
    drainFd(_notifyFd);
    drainFd(_dataFdEven);
    drainFd(_dataFdOdd);
    if (verbose) {
      printf(" ** %s: emptied input buffers **\n\r", __FUNCTION__);
    }
    rv = 0;     /* OK */
  }
  return (rv);
}

/*
 * readFrame
 *
 * RETURNS: -1 on ERROR, otherwise the number of 16-bit pixels read.
 */
int Pds::rayonix_data::readFrame(uint16_t& frameNumber, char *payload, int payloadMax,
                                 int &binning_f, int &binning_s, bool verbose) const
{
  int     dataFd;
  int     recvlen, recvsum, pixelsReceived, datalen, expectlen;
  char    mybuf[sizeof(data_footer_t)];
  data_footer_t *pNotifyMsg;
  data_footer_t *pFooter;
  uint16_t firstValue;
  char *tmpload = payload;
  int     rv = 0;

  // fetch notification
  recvlen = recvfrom(_notifyFd, mybuf, sizeof(data_footer_t), MSG_DONTWAIT, 0, 0);
  if (recvlen == sizeof(data_footer_t)) {
    pNotifyMsg = (data_footer_t *)mybuf;
    if (verbose) {
      printf("%s: recvd: epoch=%d frame=%d binning_f=%d binning_s=%d damage=0x%08x\n\r",
             __FUNCTION__, pNotifyMsg->epoch, pNotifyMsg->frameNumber,
             pNotifyMsg->binning_f, pNotifyMsg->binning_s, pNotifyMsg->damage);
    }
    binning_f = pNotifyMsg->binning_f;
    binning_s = pNotifyMsg->binning_s;
    expectlen = MAX_LINE_PIXELS * 2;

    // fetch frame data
    recvsum = 0;
    dataFd = (pNotifyMsg->frameNumber & 1) ? _dataFdOdd : _dataFdEven;
    for (int ii = 0; ii < MAX_LINE_PIXELS/binning_s; ii += binning_f) {
      recvlen = recvfrom(dataFd, tmpload, payloadMax, MSG_DONTWAIT, 0, 0);
      // verify len
      if (recvlen <= 0) {
        perror("readFrame() recvfrom");
        rv = -1;
        break;
      }
      datalen = recvlen - sizeof(data_footer_t);
      if (datalen != expectlen) {
        printf(" ** ERROR %s: data size mismatch -- recvd %d expected %d\n\r",
               __FUNCTION__, datalen, expectlen);
        rv = -1;
        break;
      }
      // verify payload footer is correct
      pFooter = (data_footer_t *)(tmpload + datalen);
      if (pFooter->lineNumber != ii) {
        printf(" ** ERROR %s: line number mismatch -- recvd %d expected %d\n\r",
               __FUNCTION__, pFooter->lineNumber, ii);
        rv = -1;
      }
      if (pFooter->frameNumber != pNotifyMsg->frameNumber) {
        printf(" ** ERROR %s: frame number mismatch -- recvd %d expected %d\n\r",
               __FUNCTION__, pFooter->frameNumber, pNotifyMsg->frameNumber);
        rv = -1;
      }
      if (pFooter->binning_f != pNotifyMsg->binning_f) {
        printf(" ** ERROR %s: binning_f mismatch -- recvd %d expected %d\n\r",
               __FUNCTION__, pFooter->binning_f, pNotifyMsg->binning_f);
        rv = -1;
      }
      if (pFooter->binning_s != pNotifyMsg->binning_s) {
        printf(" ** ERROR %s: binning_s mismatch -- recvd %d expected %d\n\r",
               __FUNCTION__, pFooter->binning_s, pNotifyMsg->binning_s);
        rv = -1;
      }

      // advance payload pointer to next line
      recvsum += datalen;
      tmpload += datalen;
      payloadMax -= datalen;
    }
    pixelsReceived = recvsum / 2;
    expectlen = ROUND_UP(MAX_FRAME_PIXELS/binning_s/binning_f, MAX_LINE_PIXELS);
    if (pixelsReceived != expectlen) {
      printf(" ** ERROR %s: received %d pixels from dataFd (expected %d)\n\r",
             __FUNCTION__, pixelsReceived, expectlen);
      rv = -1;
    }
    frameNumber = pNotifyMsg->frameNumber;
    // check frame number in data
    firstValue = *((uint16_t *) payload);

    if (firstValue != frameNumber) {
      printf("%s: frame # mismatch: notify=%hu first=%hu\n\r",
             __FUNCTION__, frameNumber, firstValue);
      rv = -1;
    }
  } else {
    printf(" *** ERROR %s: received %d bytes from notifyFd\n\r", __PRETTY_FUNCTION__, recvlen);
    rv = -1;
  }
  return (rv == 0 ? pixelsReceived : rv);
}

int createUdpSocket(int port)
{
  struct sockaddr_in myaddr; /* our address */
  int fd; /* our socket */

  /* create a UDP socket */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return 0;
  }

  /* bind the socket to any valid IP address and a specific port */
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);
  if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
    perror("bind");
    return 0;
  }
  /* set receive buffer size */
  if (setrcvbuf(fd, UDP_RCVBUF_SIZE) < 0) {
    printf("Error: Failed to set socket receive buffer to %u bytes\n\r", UDP_RCVBUF_SIZE);
    return 0;
  }
  return (fd);
}

int setrcvbuf(int socketFd, unsigned size)
{
  if (::setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF,
       (char*)&size, sizeof(size)) < 0) {
    perror("setsockopt");
    return -1;
  }
  return 0;
}
