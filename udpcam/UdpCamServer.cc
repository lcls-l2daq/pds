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
#include <sys/select.h>

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
} fccd_cmd_t;

char frameBuf[FRAMEBUF_SIZE];

static int set_cpu_affinity(int cpu_id);

Pds::UdpCamServer::UdpCamServer( const Src& client, unsigned verbosity, int dataPort, unsigned debug, int cpu0)
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
     _shutdownFlag(0),
     _cpu0(cpu0)
{
  if (_verbosity) {
    printf("%s: data port = %u\n", __FUNCTION__, _dataPort);
  }

  // allocate read task and buffer
  _readTask = new Task(TaskObject("read"));
  _frameBuffer = new vector<BufferElement>(BufferCount);

  // create completed pipe
  int err = ::pipe(_completedPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_completedPipeFd[0]);
  }

  // create routine (but do not yet call it)
  _readRoutine = new ReadRoutine(this, 0, _cpu0);

  // printf(" ** sizeof(cmdBuf) == %u  sizeof(dataBuf) == %u **\n", sizeof(cmdBuf), sizeof(dataBuf));

  // calculate xtc extent
  _xtc.extent = sizeof(FrameType) + sizeof(Xtc) + (960 * 960 * 2);
  _xtcDamaged.extent = _xtc.extent;
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

  if (_debug & UDPCAM_DEBUG_IGNORE_FRAMECOUNT) {
    printf("%s: UDPCAM_DEBUG_IGNORE_FRAMECOUNT (0x%x) is set\n",
           __FUNCTION__, UDPCAM_DEBUG_IGNORE_FRAMECOUNT);
  }
}

int Pds::UdpCamServer::payloadComplete(vector<BufferElement>::iterator buf_iter, bool missedTrigger)
{
  int rv;
  command_t sendCommand;

  sendCommand.cmd = FrameAvailable;
  sendCommand.buf_iter = buf_iter;
  sendCommand.missedTrigger = missedTrigger;

  rv = ::write(_completedPipeFd[1], &sendCommand, sizeof(sendCommand));
  if (rv == -1) {
    fprintf(stderr, "%s write error: %s\n", __FUNCTION__, strerror(errno));
  }

  return (rv);
}

static int decisleep(int count)
{
  int rv = 0;
  timespec sleepTime = {0, 100000000u};  // 0.1 sec

  while (count-- > 0) {
    if (nanosleep(&sleepTime, NULL)) {
      perror("nanosleep");
      rv = -1;  // ERROR
    }
  }
  return (rv);
}

void Pds::UdpCamServer::ReadRoutine::routine()
{
  vector<BufferElement>::iterator buf_iter;
  fd_set          fds;
  struct timeval  timeout;
  int ret;
  int fd = _server->_dataFd;
  bool lastPacket = false;
  vector<BufferElement>* buffer = _server->_frameBuffer;
  unsigned int localFrameCount = 0;
  unsigned int localPacketCount = 0;
  iov[0].iov_len =  8;
  iov[1].iov_len =  9000;

  if (_cpuAffinity >= 0) {
    if (set_cpu_affinity(_cpuAffinity) != 0) {
      printf("Error: read task set_cpu_affinity(%d) failed\n", _cpuAffinity);
    }
  }

  if (_server->verbosity()) {
    printf(" ** read task init **  (fd=%d)\n", fd);
  }

  // initialize buf_iter for 1st frame
  buf_iter = buffer->begin() + (localFrameCount % BufferCount);
  // TODO: check for _full==true before using
  buf_iter->_damaged = false;
  buf_iter->_full = true;
  while (1) {
    // select with timeout
    timeout.tv_sec  = 0;
    timeout.tv_usec = 250000;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    ret = select(fd + 1, &fds, NULL, NULL, &timeout);
    if (_server->_shutdownFlag) {
      break;        // shutdown
    }
    if (ret == 0) {
      if (_server->verbosity() > 1) {
        printf(" (timeout)\n");
      }
      continue;     // timed out, select again
    }
    iov[0].iov_base = &buf_iter->_header;
    iov[1].iov_base = buf_iter->_rawData + (localPacketCount * PacketDataSize);
    ret = readv(fd, iov, 2);
    if (ret == -1) {
      perror("readv");
      break;        // shutdown
    }
    unsigned pktidx = (unsigned)buf_iter->_header.packetIndex & 0xff;
    if (!(_server->_debug & UDPCAM_DEBUG_IGNORE_PACKET_CNT)) {
      if (pktidx != localPacketCount) {
        printf("Error: received packet index %u, expected %u\n", pktidx, localPacketCount);
        buf_iter->_damaged = true;
      }
    }
    if (pktidx == LastPacketIndex) {
      lastPacket = true;
      if (_server->verbosity() > 1) {
        printf(" ** last packet **  ");
        printf("(frameIndex = %hu)\n", buf_iter->_header.frameIndex);
      }
      _server->payloadComplete(buf_iter, false);
      // advance buf_iter for next frame
      buf_iter = buffer->begin() + (localFrameCount++ % BufferCount);
      localPacketCount = 0; // zero packet count for next frame
      buf_iter->_damaged = false;   // clear damage for next frame
    } else {
      ++ localPacketCount;
    }
  }

  printf(" ** read task shutdown **\n");
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

  // ensure that pipes are empty
  int nbytes;
  char bucket;
  if (_completedPipeFd[0]) {
    if (ioctl(_completedPipeFd[0], FIONREAD, &nbytes) == -1) {
      perror("ioctl");
    } else if (nbytes > 0) {
      printf("%s: draining %d bytes from completed pipe\n", __FUNCTION__, nbytes);
      while (nbytes-- > 0) {
        ::read(_completedPipeFd[0], &bucket, 1);
      }
    }
  }

  // ensure that buffers are cleared
  vector<BufferElement>::iterator buf_iter;
  for (buf_iter = _frameBuffer->begin(); buf_iter != _frameBuffer->end(); buf_iter++) {
    buf_iter->_full = false;
  }

  // create UDP socket
  _dataFd = createUdpSocket(_dataPort);
  if (_dataFd <= 0) {
    ++ numErrs;
  }
  if (_verbosity) {
    if (_dataFd) {
      printf("   createUdpSocket(%d) returned %d\n\r", _dataPort, _dataFd);
    } else {
      printf("Error: createUdpSocket(%d) returned 0\n\r", _dataPort);
    }
  }

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

  // create reader thread
  _readTask->call(_readRoutine);
  decisleep(1);

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

  shutdown();
  // give task a bit of time to shutdown
  decisleep(3);

  // close UDP socket
  close (_dataFd);

  return (0);
}

unsigned Pds::UdpCamServer::endrun(void)
{
  return (0);
}

int Pds::UdpCamServer::fetch( char* payload, int flags )
{
  command_t receiveCommand;
  char msgBuf[100];
  enum {Ignore=-1};
  int errs = 0;
  int rv;

  if (verbosity() > 1) {
    printf(" ** UdpCamServer::fetch() **\n");
  }

  rv = ::read(_completedPipeFd[0], &receiveCommand, sizeof(receiveCommand));
  if (rv == -1) {
    perror("read");
    ++ errs;
  }

  if (_outOfOrder) {
    // error condition is latched
    return (Ignore);
  }

  uint16_t fx = receiveCommand.buf_iter->_header.frameIndex;

  if (verbosity()) {
    printf("fetch: frame = %hu  ", fx);
    if (receiveCommand.buf_iter->_damaged) {
      printf("** DAMAGED **");
    }
    printf("\n");
  }

  // handle damage marked in frame
  if (receiveCommand.buf_iter->_damaged) {
    ++ errs;
  }

  // handle frame counter
  if (_resetHwCount) {
    _count = 0;
    _countOffset = fx - 1;
    _resetHwCount = false;
    _prevPacket = _currPacket;
  }

  // update frame counter
  ++_count;
  uint16_t sum16 = (uint16_t)(_count + _countOffset);

  if (!(_debug & UDPCAM_DEBUG_IGNORE_FRAMECOUNT)) {
    // check for out-of-order condition
    if (fx != sum16) {
      snprintf(msgBuf, sizeof(msgBuf), "Fccd960: hw count (%hu) != sw count (%hu) + offset (%u) == (%hu)\n",
              fx, _count, _countOffset, sum16);
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

  return (_xtc.extent);
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

#include <pthread.h>

static int set_cpu_affinity(int cpu_id)
{
  int rv;
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);

  rv = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (rv != 0) {
    perror("pthread_setaffinity_np");
  }
  return (rv);
}
