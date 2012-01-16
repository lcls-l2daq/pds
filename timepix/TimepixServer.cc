#include "TimepixServer.hh"
#include "pds/xtc/XtcType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

#include "mpxmodule.h"

using namespace Pds;

#define PROFILE_MAX (USHRT_MAX+1)

timespec _profile1[PROFILE_MAX];
timespec _profile2[PROFILE_MAX];
timespec _profile3[PROFILE_MAX];
timespec _profile4[PROFILE_MAX];
timespec _profile5[PROFILE_MAX];
timespec _profile6[PROFILE_MAX];

static int setFdFlag(int fd, int flag);   // forward declaration

Pds::TimepixServer::TimepixServer( const Src& client, unsigned moduleId, unsigned verbosity, unsigned debug)
   : _xtc( _timepixDataType, client ),
//   _occSend(NULL),
     _moduleId(moduleId),
     _verbosity(verbosity),
     _debug(debug),
     _outOfOrder(0),
     _triggerConfigured(false),
     _timepix(NULL),
     _buffer(new vector<BufferElement>(BufferDepth)),
     _readTask(new Task(TaskObject("readframe"))),
     _decodeTask(new Task(TaskObject("decodeframe")))
{
  // calculate xtc extent
  _xtc.extent = sizeof(TimepixDataType) + sizeof(Xtc) + TIMEPIX_DECODED_DATA_BYTES;

  // create completed pipe
  int err = ::pipe(_completedPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_completedPipeFd[0]);

    if (_debug & TIMEPIX_DEBUG_NONBLOCK) {
      // make writes to completed pipe non-blocking
      if (setFdFlag(_completedPipeFd[1], O_NONBLOCK) == -1) {
        fprintf(stderr, "%s pipe error\n", __FUNCTION__);
      }
    }
  }
  // create raw pipe
  err = ::pipe(_rawPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    if (_debug & TIMEPIX_DEBUG_NONBLOCK) {
      // make writes to raw pipe non-blocking
      if (setFdFlag(_rawPipeFd[1], O_NONBLOCK) == -1) {
        fprintf(stderr, "%s pipe error\n", __FUNCTION__);
      }
    }
    // create routines
   _readRoutine = new ReadRoutine(this, _rawPipeFd[1]);
   _decodeRoutine = new DecodeRoutine(this, _rawPipeFd[0]);
    // call routines in separate tasks
    _readTask->call(_readRoutine);
    _decodeTask->call(_decodeRoutine);
  }

  if (_debug & TIMEPIX_DEBUG_PROFILE) {
    printf("%s: TIMEPIX_DEBUG_PROFILE (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_PROFILE);
  }
  if (_debug & TIMEPIX_DEBUG_NONBLOCK) {
    printf("%s: TIMEPIX_DEBUG_NONBLOCK (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_NONBLOCK);
  }
  if (_debug & TIMEPIX_DEBUG_CONVERT) {
    printf("%s: TIMEPIX_DEBUG_CONVERT (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_CONVERT);
  }
}

int Pds::TimepixServer::payloadComplete(vector<BufferElement>::iterator buf_iter)
{
  int rv;
  command_t sendCommand;

  sendCommand.cmd = FrameAvailable;
  sendCommand.buf_iter = buf_iter;

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

void Pds::TimepixServer::ReadRoutine::routine()
{
  uint32_t ss;
  int lostRowBuf;
  vector<BufferElement>::iterator buf_iter;
  command_t sendCommand;
  timespec time1, time2;

  // Wait for _timepix to be set
  while (_server->_timepix == NULL) {
    decisleep(1);
  }

  while (1) {
    for (buf_iter = _server->_buffer->begin(); buf_iter != _server->_buffer->end(); buf_iter++) {

do_over:
      // wait for trigger to be configured
      while (!_server->_triggerConfigured) {
        decisleep(1);
      }

      // trigger is configured
      // wait for new frame
      while (!_server->_timepix->newFrame()) {
        if (!_server->_triggerConfigured) {
          goto do_over;
        }
      }

      // new frame is available...
      if (buf_iter->_full) {      // is this buffer already full?
        fprintf(stderr, "Error: buffer overflow in %s\n", __PRETTY_FUNCTION__);
        return;                   // yes: exit
      } else {
        buf_iter->_full = true;   // no: mark this buffer as full
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &time1);          // TIMESTAMP 1
      }

      // read frame
      int rv = _server->_timepix->readMatrixRawPlus(buf_iter->_rawData, &ss, &lostRowBuf,
                                                    &buf_iter->_header._frameCounter,
                                                    &buf_iter->_header._timestamp);
      if (rv) {
        fprintf(stderr, "Error: readMatrixRawPlus() failed\n");
      } else if ((ss != TIMEPIX_RAW_DATA_BYTES) || (lostRowBuf != 0)) {
        fprintf(stderr, "Error: readMatrixRawPlus: sz=%u lost_rows=%d\n", ss, lostRowBuf);
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &time2);          // TIMESTAMP 2
        _profile1[buf_iter->_header._frameCounter] = time1;
        _profile2[buf_iter->_header._frameCounter] = time2;
      }

      // fill in lost_rows
      buf_iter->_header._lostRows = lostRowBuf;

      // send notification
      sendCommand.cmd = FrameAvailable;
      sendCommand.buf_iter = buf_iter;
      if (::write(_writeFd, &sendCommand, sizeof(sendCommand)) == -1) {
        fprintf(stderr, "%s write error: %s\n", __PRETTY_FUNCTION__, strerror(errno));
      }
    } // end of for
  } // end of while
}

void Pds::TimepixServer::DecodeRoutine::routine()
{
  // vector<BufferElement>::iterator bb;
  command_t     receiveCommand;

  // Wait for _timepix to be set
  while (_server->_timepix == NULL) {
    decisleep(1);
  }
  // wait for trigger to be configured
  while (!_server->_triggerConfigured) {
    decisleep(1);
  }
  // trigger is configured
  while (1) {
    // read from pipe
    int length = ::read(_readFd, &receiveCommand, sizeof(receiveCommand));
    if (length != sizeof(receiveCommand)) {
      fprintf(stderr, "Error: read() returned %d in %s\n", length, __PRETTY_FUNCTION__);
    }

    if (receiveCommand.cmd == FrameAvailable) {
      vector<BufferElement>::iterator buf_iter = receiveCommand.buf_iter;

      // validate buffer
      if (!buf_iter->_full) {
        fprintf(stderr, "Error: buffer underflow in %s\n", __PRETTY_FUNCTION__);
        return;                   // yes: exit
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &_profile3[buf_iter->_header._frameCounter]);    // TIMESTAMP 3
      }

      if (_server->_debug & TIMEPIX_DEBUG_CONVERT) {
        // decode to pixels
        _server->_timepix->decode2Pixels(buf_iter->_rawData, buf_iter->_pixelData);
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &_profile4[buf_iter->_header._frameCounter]);    // TIMESTAMP 4
      }

    } else {
      fprintf(stderr, "Error: unrecognized cmd (%d) in %s\n",
              (int)receiveCommand.cmd, __PRETTY_FUNCTION__);
      continue;
    }

    // send notification
    _server->payloadComplete(receiveCommand.buf_iter);
  }
}

unsigned Pds::TimepixServer::configure(const TimepixConfigType& config)
{
  // int rv;
  unsigned int configReg;
  unsigned numErrs = 0;

  _count = 0;

  if (_timepix == NULL) {
    fprintf(stderr, "Error: _timepix is NULL in %s\n", __PRETTY_FUNCTION__);
    return (1);
  }

  if (_timepix->resetFrameCounter()) {
    fprintf(stderr, "Error: resetFrameCounter() failed in %s\n", __FUNCTION__);
    ++numErrs;
  }

  _readoutSpeed = config.readoutSpeed();
  _triggerMode = config.triggerMode();
  _shutterTimeout = config.shutterTimeout();

  _dac0[0]  = config.dac0Ikrum();
  _dac0[1]  = config.dac0Disc();
  _dac0[2]  = config.dac0Preamp();
  _dac0[3]  = config.dac0BufAnalogA();
  _dac0[4]  = config.dac0BufAnalogB();
  _dac0[5]  = config.dac0Hist();
  _dac0[6]  = config.dac0ThlFine();
  _dac0[7]  = config.dac0ThlCourse();
  _dac0[8]  = config.dac0Vcas();
  _dac0[9]  = config.dac0Fbk();
  _dac0[10] = config.dac0Gnd();
  _dac0[11] = config.dac0Ths();
  _dac0[12] = config.dac0BiasLvds();
  _dac0[13] = config.dac0RefLvds();

  _dac1[0]  = config.dac1Ikrum();
  _dac1[1]  = config.dac1Disc();
  _dac1[2]  = config.dac1Preamp();
  _dac1[3]  = config.dac1BufAnalogA();
  _dac1[4]  = config.dac1BufAnalogB();
  _dac1[5]  = config.dac1Hist();
  _dac1[6]  = config.dac1ThlFine();
  _dac1[7]  = config.dac1ThlCourse();
  _dac1[8]  = config.dac1Vcas();
  _dac1[9]  = config.dac1Fbk();
  _dac1[10] = config.dac1Gnd();
  _dac1[11] = config.dac1Ths();
  _dac1[12] = config.dac1BiasLvds();
  _dac1[13] = config.dac1RefLvds();

  _dac2[0]  = config.dac2Ikrum();
  _dac2[1]  = config.dac2Disc();
  _dac2[2]  = config.dac2Preamp();
  _dac2[3]  = config.dac2BufAnalogA();
  _dac2[4]  = config.dac2BufAnalogB();
  _dac2[5]  = config.dac2Hist();
  _dac2[6]  = config.dac2ThlFine();
  _dac2[7]  = config.dac2ThlCourse();
  _dac2[8]  = config.dac2Vcas();
  _dac2[9]  = config.dac2Fbk();
  _dac2[10] = config.dac2Gnd();
  _dac2[11] = config.dac2Ths();
  _dac2[12] = config.dac2BiasLvds();
  _dac2[13] = config.dac2RefLvds();

  _dac3[0]  = config.dac3Ikrum();
  _dac3[1]  = config.dac3Disc();
  _dac3[2]  = config.dac3Preamp();
  _dac3[3]  = config.dac3BufAnalogA();
  _dac3[4]  = config.dac3BufAnalogB();
  _dac3[5]  = config.dac3Hist();
  _dac3[6]  = config.dac3ThlFine();
  _dac3[7]  = config.dac3ThlCourse();
  _dac3[8]  = config.dac3Vcas();
  _dac3[9]  = config.dac3Fbk();
  _dac3[10] = config.dac3Gnd();
  _dac3[11] = config.dac3Ths();
  _dac3[12] = config.dac3BiasLvds();
  _dac3[13] = config.dac3RefLvds();

  // Medipix device DACs configuration
  if (_timepix->setFsr(0, _dac0) ) {
    fprintf(stderr, "Error: setFsr() chip 0 failed\n");
    ++numErrs;
  }
  if (_timepix->setFsr(1, _dac1) ) {
    fprintf(stderr, "Error: setFsr() chip 1 failed\n");
    ++numErrs;
  }
  if (_timepix->setFsr(2, _dac2) ) {
    fprintf(stderr, "Error: setFsr() chip 2 failed\n");
    ++numErrs;
  }
  if (_timepix->setFsr(3, _dac3) ) {
    fprintf(stderr, "Error: setFsr() chip 3 failed\n");
    ++numErrs;
  }

  if (_timepix->readReg(MPIX2_CONF_REG_OFFSET, &configReg) == 0) {
    configReg &= ~MPIX2_CONF_TIMER_USED;      // Reset 'use Timer'

    configReg |= MPIX2_CONF_EXT_TRIG_ENABLE;  // Enable external trigger

    configReg &= ~MPIX2_CONF_EXT_TRIG_FALLING_EDGE; // Trigger on rising edge

    configReg &= ~MPIX2_CONF_EXT_TRIG_INHIBIT; // Ready for next (external) trigger

    if (_readoutSpeed == Timepix::ConfigV1::ReadoutSpeed_Fast) {
      configReg |= MPIX2_CONF_RO_CLOCK_125MHZ;  // Enable high speed chip readout
    }

    if (_timepix->writeReg(MPIX2_CONF_REG_OFFSET, configReg)) {
      fprintf(stderr, "Error: writeReg(MPIX2_CONF_REG_OFFSET) failed\n");
      ++numErrs;
    }
  } else {
    fprintf(stderr, "Error: readReg(MPIX2_CONF_REG_OFFSET) failed\n");
    ++numErrs;
  }

  if (_timepix->readReg(MPIX2_CONF_REG_OFFSET, &configReg) == 0) {
    printf("Timepix configuration register: 0x%08x\n", configReg);
  }

  // verify that chipCount() returns 4
  int ndevs = _timepix->chipCount();
  if (ndevs == 4) {
    fprintf(stdout, "Timepix chip count = 4\n");
  } else {
    fprintf(stderr, "Error: Timepix chip count = %d\n", ndevs);
    ++numErrs;
  }

  if (numErrs > 0) {
      fprintf(stderr, "Timepix: Failed to configure.\n");
      // send occurrence TODO
  } else {
    // enable polling for new frames
    _triggerConfigured = true;
  }

  return (numErrs);
}

unsigned Pds::TimepixServer::unconfigure(void)
{
  _count = 0;
  // disable polling for new frames
  _triggerConfigured = false;

  // reinitialize Timepix
  if (_timepix->warmup(true)) {
    fprintf(stderr, "Error: warmup() failed in %s\n", __FUNCTION__);
  }

  return (0);
}

int Pds::TimepixServer::fetch( char* payload, int flags )
{
  command_t     receiveCommand;
  timespec time5;

  if (_outOfOrder) {
    // error condition is latched
    return (-1);
  }

  if (_debug & TIMEPIX_DEBUG_PROFILE) {
    clock_gettime(CLOCK_REALTIME, &time5);    // TIMESTAMP 5
  }

  TimepixDataType *frame = (TimepixDataType *)(payload + sizeof(Xtc));

  memcpy(payload, &_xtc, sizeof(Xtc));  // xtc extent calculated in TimepixServer constructor

  // read from pipe
  int length = ::read(_completedPipeFd[0], &receiveCommand, sizeof(receiveCommand));

  if (length != sizeof(receiveCommand)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", length, __FUNCTION__);
    return (-1);    // ERROR
  } else if (receiveCommand.cmd == FrameAvailable) {

    if (!receiveCommand.buf_iter->_full) {     // is this buffer empty?
      fprintf(stderr, "Error: buffer underflow in %s\n", __PRETTY_FUNCTION__);
      // TODO damage
    }

    frame->_lostRows = receiveCommand.buf_iter->_header._lostRows;
    frame->_frameCounter = receiveCommand.buf_iter->_header._frameCounter;
    frame->_timestamp = receiveCommand.buf_iter->_header._timestamp;

    ++_count;

    // check for out-of-order condition
    uint16_t count16 = _count & 0xffff;
    if (count16 != frame->_frameCounter) {
      fprintf(stderr, "Error: sw count=%hu does not match hw frameCounter=%hu\n",
              count16, frame->_frameCounter);
      // TODO damage
    }

    // FIXME copy pixels to payload (message queue?)

    // mark buffer as empty
    receiveCommand.buf_iter->_full = false;

    if (_debug & TIMEPIX_DEBUG_PROFILE) {
      // TIMESTAMP 6
      clock_gettime(CLOCK_REALTIME, &_profile6[receiveCommand.buf_iter->_header._frameCounter]);
      // fill in timestamp 5
      _profile5[receiveCommand.buf_iter->_header._frameCounter] = time5;
    }

    return (_xtc.extent);

  } else {
    printf("Unknown command (0x%x) in %s\n", (int)receiveCommand.cmd, __FUNCTION__);
    return (-1);
  }

  return (0);
}

unsigned TimepixServer::count() const
{
  return (_count-1);
}

unsigned TimepixServer::verbosity() const
{
  return (_verbosity);
}

unsigned TimepixServer::moduleId() const
{
  return (_moduleId);
}

unsigned TimepixServer::debug() const
{
  return (_debug);
}

#if 0
void TimepixServer::setOccSend(TimepixOccurrence* occSend)
{
  _occSend = occSend;
}
#endif

void TimepixServer::setTimepix(timepix_dev* timepix)
{
  _timepix = timepix;
}

static int setFdFlag(int fd, int flag)
{
  int fdflags;
  int rv = -1;

  fdflags = ::fcntl(fd, F_GETFL);
  if (fdflags == -1) {
    perror("fcntl");
  } else {
    if (::fcntl(fd, F_SETFL, fdflags | flag) == -1) {
      perror("fcntl");
    } else {
      rv = 0;
    }
  }
  return (rv);
}
