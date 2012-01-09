#include "TimepixServer.hh"
#include "pds/xtc/XtcType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "mpxmodule.h"

using namespace Pds;

Pds::TimepixServer::TimepixServer( const Src& client )
   : _xtc( _timepixDataType, client ),
//   _occSend(NULL),
     _outOfOrder(0),
     _triggerConfigured(false),
     _timepix(NULL),
     _task(new Task(TaskObject("waitframe")))
{
  // calculate xtc extent
  _xtc.extent = sizeof(TimepixDataType) + sizeof(Xtc) + TIMEPIX_RAW_DATA_BYTES;

  // create pipe
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_pfd[0]);
    // call routine() in separate task
    _task->call(this);
  }
}

int Pds::TimepixServer::payloadComplete(void)
{
  const Command cmd = FrameAvailable;
  int rv = ::write(_pfd[1],&cmd,sizeof(cmd));
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

void Pds::TimepixServer::routine()
{
  // Wait for _timepix to be set
  while (_timepix == NULL) {
    decisleep(1);
  }

  while (1) {
    if (!_triggerConfigured) {
      decisleep(1);
      continue;
    }

    // check for new frame
    if (_timepix->newFrame()) {
      // new frame is available...
      // send notification
      payloadComplete();
      // don't need to clear it 'cause it already has
    } else {
      // no new frame is available
      continue;
    }
  } // end of while(1)
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
  printf("Entered %s\n", __PRETTY_FUNCTION__);  // FIXME temp
  // disable polling for new frames
  _triggerConfigured = false;
  return (0);
}

int Pds::TimepixServer::fetch( char* payload, int flags )
{
  unsigned int  sizeBuf;
  int           lostRowBuf;

  if (_outOfOrder) {
    // error condition is latched
    return (-1);
  }

  TimepixDataType *frame = (TimepixDataType *)(payload + sizeof(Xtc));

  memcpy(payload, &_xtc, sizeof(Xtc));  // xtc extent calculated in TimepixServer constructor

  // read from pipe
  int length = ::read(_pfd[0], &_cmd, sizeof(_cmd));

  if (length != sizeof(_cmd)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", length, __FUNCTION__);
    return (-1);    // ERROR
  } else if (_cmd == FrameAvailable) {
    // read the data from Timepix
    if (_timepix) {
      // get raw frame
      if (_timepix->readMatrixRaw((unsigned char *)(frame+1), &sizeBuf, &lostRowBuf) ) {
        fprintf(stderr, "Error: readMatrixRaw() failed\n");
        lostRowBuf = 512;
        // TODO damage
      } else if ((sizeBuf != TIMEPIX_RAW_DATA_BYTES) || (lostRowBuf != 0)) {
        fprintf(stderr, "Error: readMatrixRaw: sz=%u lost_rows=%d\n",
                sizeBuf, lostRowBuf);
        // TODO damage
      }
      frame->_lostRows = (uint16_t)lostRowBuf;
      frame->_frameCounter = _timepix->lastFrameCount();
      frame->_timestamp = _timepix->lastClockTick();
      ++_count;

      // check for out-of-order condition
      if (_count != frame->_frameCounter) {
        fprintf(stderr, "Error: sw count=%hu does not match hw frameCounter=%hu\n",
                _count, frame->_frameCounter);
        // TODO damage
      }

      return (_xtc.extent);

    } else {
      fprintf(stderr, "Error: _timepix is NULL in %s\n", __FUNCTION__);
      return (-1);
    }

  } else {
    printf("Unknown command (0x%x) in %s\n", _cmd, __FUNCTION__);
    return (-1);
  }

  return (0);
}

unsigned TimepixServer::count() const
{
  return (_count-1);
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
