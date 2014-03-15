#include "UdpCamServer.hh"
#include "pds/xtc/XtcType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <sys/uio.h>

using namespace Pds;

// #define RESET_COUNT 0x80000000

// forward declarations
int createUdpSocket(int port);
int setrcvbuf(int socketFd, unsigned size);
int drainFd(int fdx);

// globals

struct iovec iov[2];

typedef struct {
  char packetIndex;
  char f0;
  char f1;
  char f2;
  char f3;
  char f4;
  uint16_t frameIndex;
} cmd_t;

cmd_t cmdBuf;           // 8 bytes

char dataBuf[64 * 1024];

Pds::UdpCamServer::UdpCamServer( const Src& client, unsigned verbosity, int dataPort, unsigned debug)
   : _xtc( _frameType, client ),
     _xtcDamaged( _frameType, client ),
     _count(0),
     _resetHwCount(true),
     _countOffset(0),
     _occSend(NULL),
     _verbosity(verbosity),
     _dataPort(dataPort),
     _debug(debug),
     _outOfOrder(0),
     _frameStarted(false),
     _shutdownFlag(0)
{
  if (_verbosity) {
    printf("%s: data port = %u\n", __FUNCTION__, _dataPort);
  }

  // create UDP socket
  _dataFd = createUdpSocket(_dataPort);
  if (_verbosity) {
    if (_dataFd) {
      printf("   createUdpSocket(%d) returned %d\n\r", _dataPort, _dataFd);
    } else {
      printf("Error: createUdpSocket(%d) returned 0\n\r", _dataPort);
    }
  }

  // set up iov[0]
  iov[0].iov_base = &cmdBuf;
  iov[0].iov_len =  sizeof(cmdBuf);

  iov[1].iov_base = &dataBuf;   // FIXME temporary location
  iov[1].iov_len =  sizeof(dataBuf);

  // printf(" ** sizeof(cmdBuf) == %u  sizeof(dataBuf) == %u **\n", sizeof(cmdBuf), sizeof(dataBuf));

  // calculate xtc extent
  _xtc.extent = sizeof(FrameType) + sizeof(Xtc) + (960 * 960 * 2);
  _xtcDamaged.extent = _xtc.extent;
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

  // setup to read from socket
  fd(_dataFd);

  if (_debug & UDPCAM_DEBUG_IGNORE_FRAMECOUNT) {
    printf("%s: UDPCAM_DEBUG_IGNORE_FRAMECOUNT (0x%x) is set\n",
           __FUNCTION__, UDPCAM_DEBUG_IGNORE_FRAMECOUNT);
  }
}

unsigned Pds::UdpCamServer::configure()
{
  char msgBuf[100];
  unsigned numErrs = 0;

  _count = 0;
  _countOffset = 0;
  _resetHwCount = true;
//_missedTriggerCount = 0;
  _shutdownFlag = 0;

  // do init
  drainFd(fd());

  if (numErrs > 0) {
    // failed initialization
    snprintf(msgBuf, sizeof(msgBuf), "UdpCam init failed\n");
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage(msgBuf);
    }
    return (1);
  }

  if (numErrs > 0) {
    fprintf(stderr, "UdpCam: Failed to configure.\n");
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage("UdpCam: Failed to configure.\n");
    }
  } else {
    printf("UdpCam: Configure OK\n");
  }

  return (numErrs);
}

unsigned Pds::UdpCamServer::unconfigure(void)
{
  _count = 0;
  // set up to read from socket
  fd(_dataFd);
  return (0);
}

unsigned Pds::UdpCamServer::endrun(void)
{
  return (0);
}

int Pds::UdpCamServer::fetch( char* payload, int flags )
{
  char msgBuf[100];
  bool lastPacket = false;
  bool packetSequenceError = false;
  enum {Ignore=-1};
  int errs = 0;

  if (verbosity() > 1) {
    printf(" ** UdpCamServer::fetch() **\n");
  }

  if (_outOfOrder) {
    // error condition is latched
    return (-1);
  }

  if (!_frameStarted) {
    _goodPacketCount = 0;
  }

  // read cmd+data
  unsigned uu = readv(fd(), iov, 2);
  if (verbosity() > 1) {
    printf("  readv() returned %u\n", uu);
  }
  _currPacket = (unsigned)cmdBuf.packetIndex & 0xff;
  if (verbosity()) {
    unsigned printF0 = (unsigned)cmdBuf.f0 & 0xff;
    printf("  _currPacket=%u  f0=0x%02x frameIndex=%hu\n", _currPacket, printF0, cmdBuf.frameIndex);
  }

  if (_currPacket == LastPacketIndex) {
    lastPacket = true;
    if (verbosity()) {
      printf(" ** last packet **\n");
    }
  }

  // handle frame counter (revisit for partial frames)
  if (_resetHwCount) {
    _count = 0;
    _countOffset = cmdBuf.frameIndex - 1;
    _resetHwCount = false;
    _prevPacket = _currPacket;
  } else {
    // handle packet counter
    if (_frameStarted && (_currPacket != _prevPacket + 1)) {
      printf("Error: prev packet = %u, curr packet = %u\n", _prevPacket, _currPacket);
      packetSequenceError = true;   // affects packet count
    }
    _prevPacket = _currPacket;
  }

  if (lastPacket) {
    _frameStarted = false;
    // update frame counter
    ++_count;
    uint16_t sum16 = (uint16_t)(_count + _countOffset);

    if (!(_debug & UDPCAM_DEBUG_IGNORE_FRAMECOUNT)) {
      // check for out-of-order condition
      if (cmdBuf.frameIndex != sum16) {
        snprintf(msgBuf, sizeof(msgBuf), "Fccd960: hw count (%hu) != sw count (%hu) + offset (%u) == (%hu)\n",
                cmdBuf.frameIndex, _count, _countOffset, sum16);
        fprintf(stderr, "%s: %s", __FUNCTION__, msgBuf);
        // latch error
        _outOfOrder = 1;
        if (_occSend) {
          // send occurrence
          _occSend->userMessage(msgBuf);
          _occSend->outOfOrder();
        }
        return (Ignore);
      }
    }
  } else {
    _frameStarted = true;
    if (!packetSequenceError) {
      ++ _goodPacketCount;
    }
  }

  if (lastPacket) {
    if (_goodPacketCount != LastPacketIndex) {
      printf("Error: Fccd960 received %u packets, expected %u (frame# = %u)\n",
             _goodPacketCount, LastPacketIndex, _count);
      ++ errs;
    }
    // copy xtc to payload
    if (errs > 0) {
      // ...damaged
      printf(" ** frame %u damaged **\n", _count);
      memcpy(payload, &_xtcDamaged, sizeof(Xtc));
    } else {
      // ...undamaged
      memcpy(payload, &_xtc, sizeof(Xtc));
    }

    // reorder and copy pixels to payload
    // FIXME

    if (verbosity() > 1) {
      printf(" ** goodPacketCount = %u **\n", _goodPacketCount);
    }
  }

  return (lastPacket ? _xtc.extent : Ignore);
}

unsigned UdpCamServer::count() const
{
  return (_count-1);
}

unsigned UdpCamServer::verbosity() const
{
  return (_verbosity);
}

unsigned UdpCamServer::debug() const
{
  return (_debug);
}

void UdpCamServer::setOccSend(UdpCamOccurrence* occSend)
{
  _occSend = occSend;
}

void UdpCamServer::shutdown()
{
  printf("\n ** UdpCamServer::shutdown() **\n");
  _shutdownFlag = 1;
}

int createUdpSocket(int port)
{
  struct sockaddr_in myaddr; /* our address */
  int fd; /* our socket */

  printf(" ** createUdpSocket(%d) **\n", port);

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

#define DISCARD_SIZE (64*1024)

char discardBuf[DISCARD_SIZE];

int drainFd(int fdx)
{
  int rv;

  while ((rv = recvfrom(fdx, discardBuf, DISCARD_SIZE, MSG_DONTWAIT, 0, 0)) > 0) {
    ;
  }
  return (rv);
}
