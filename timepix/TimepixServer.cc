#include "TimepixServer.hh"
#include "pds/xtc/XtcType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

#include "mpxmodule.h"

using namespace Pds;

#define RESET_COUNT 0x80000000

#define PROFILE_MAX (USHRT_MAX+1)

timespec _profile1[PROFILE_MAX];
timespec _profile2[PROFILE_MAX];
timespec _profile3[PROFILE_MAX];
timespec _profile4[PROFILE_MAX];
timespec _profile5[PROFILE_MAX];
timespec _profile6[PROFILE_MAX];
uint32_t _profileHwTimestamp[PROFILE_MAX];

static void shuffleTimepixQuad(int16_t *dst, int16_t *src);

Pds::TimepixServer::TimepixServer( const Src& client, unsigned moduleId, unsigned verbosity, unsigned debug, char *threshFile,
                                   char *testImageFile, int readCpu, int decodeCpu)
   : _xtc( _timepixDataType, client ),
     _xtcDamaged( _timepixDataType, client ),
     _count(0),
     _resetHwCount(true),
     _countOffset(0),
     _occSend(NULL),
     _expectedDiff(0),
     _resetTimestampCount(4),
     _moduleId(moduleId),
     _verbosity(verbosity),
     _debug(debug),
     _outOfOrder(0),
     _badFrame(0),
     _uglyFrame(0),
     _triggerConfigured(false),
     _profileCollected(false),
     _timepix(NULL),
     _buffer(new vector<BufferElement>(BufferDepth)),
     _readTask(new Task(TaskObject("readframe"))),
     _decodeTask(new Task(TaskObject("decodeframe"))),
     _shutdownFlag(0),
     _pixelsCfg(NULL),
     _threshFile(threshFile),
     _testData(NULL),
     _readTaskState(TaskShutdown),
     _decodeTaskState(TaskShutdown),
     _relaxd(NULL),
     _readCpu(readCpu),
     _decodeCpu(decodeCpu)
{
  // calculate aux data xtc extent
  _xtc.extent = sizeof(TimepixDataType) + sizeof(Xtc) + Pds::Timepix::DataV1::DecodedDataBytes;

  // alternate aux data xtc is used for damaged events
  _xtcDamaged.extent = sizeof(TimepixDataType) + sizeof(Xtc) + Pds::Timepix::DataV1::DecodedDataBytes;
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

  // create completed pipe
  int err = ::pipe(_completedPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_completedPipeFd[0]);
  }

  // create raw pipe
  err = ::pipe(_rawPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // create routines (but do not yet call them)
   _readRoutine = new ReadRoutine(this, _rawPipeFd[1]);
   _decodeRoutine = new DecodeRoutine(this, _rawPipeFd[0]);
  }

  if (_debug & TIMEPIX_DEBUG_PROFILE) {
    printf("%s: TIMEPIX_DEBUG_PROFILE (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_PROFILE);
  }
  if (_debug & TIMEPIX_DEBUG_TIMECHECK) {
    printf("%s: TIMEPIX_DEBUG_TIMECHECK (0x%x) is set (ignored)\n",
           __FUNCTION__, TIMEPIX_DEBUG_TIMECHECK);
  }
  if (_debug & TIMEPIX_DEBUG_NOCONVERT) {
    printf("%s: TIMEPIX_DEBUG_NOCONVERT (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_NOCONVERT);
  }
  if (_debug & TIMEPIX_DEBUG_IGNORE_FRAMECOUNT) {
    printf("%s: TIMEPIX_DEBUG_IGNORE_FRAMECOUNT (0x%x) is set\n",
           __FUNCTION__, TIMEPIX_DEBUG_IGNORE_FRAMECOUNT);
  }

  if (threshFile) {
    FILE* fp = fopen(threshFile, "r");
    if (fp == NULL) {
      perror("fopen");
    } else {
      _pixelsCfg = new uint8_t[TimepixConfigType::PixelThreshMax];
      if (fread(_pixelsCfg, TimepixConfigType::PixelThreshMax, 1, fp) != 1) {
        perror("fread");
        delete[] _pixelsCfg;
        _pixelsCfg = NULL;
      } else {
        for (int jj=0; jj < TimepixConfigType::PixelThreshMax; jj++) {
          // ensure that TOT mode is set
          _pixelsCfg[jj] &= ~TPX_CFG8_MODE_MASK;
          _pixelsCfg[jj] |= (TPX_MODE_TOT << TPX_CFG8_MODE_MASK_SHIFT);
        }
      }
      fclose(fp);
    }
  }
#if 0
  if (testImageFile) {
    FILE* fp = fopen(testImageFile, "r");
    if (fp == NULL) {
      perror("fopen");
    } else {
      _testData = new int16_t[512*512];
      // discard 3 words of header, keep 512*512 words of test data
      if ((fread(_testData, 2, 3, fp) != 3) ||
          (fread(_testData, 2, 512*512, fp) != 512*512)) {
        perror("fread");
        delete[] _testData;
        _testData = NULL;
        printf("Error: Reading test image from %s failed\n", testImageFile);
      } else {
        printf("Reading test image from %s complete\n", testImageFile);
      }
      fclose(fp);
    }
  }
#endif
}

uint8_t *Pds::TimepixServer::pixelsCfg()
{
  return _pixelsCfg;
}

int Pds::TimepixServer::payloadComplete(vector<BufferElement>::iterator buf_iter, bool missedTrigger)
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

void Pds::TimepixServer::ReadRoutine::routine()
{
  uint32_t ss;
  int lostRowBuf;
  vector<BufferElement>::iterator buf_iter;
  command_t sendCommand;
  timespec time1, time2;

  // Wait for _timepix to be set
  _server->readTaskState(TaskInit);
  printf(" ** read task init **\n");
  while (_server->_timepix == NULL) {
    if (_server->_outOfOrder || _server->_shutdownFlag) {
      goto read_shutdown;
    }
    decisleep(1);
  }

  // wait for trigger to be configured
  _server->readTaskState(TaskWaitConfigure);
  while (!_server->_triggerConfigured) {
    decisleep(1);
    if (_server->_outOfOrder || _server->_shutdownFlag) {
      goto read_shutdown;
    }
  }

  while (1) {
    for (buf_iter = _server->_buffer->begin(); buf_iter != _server->_buffer->end(); buf_iter++) {

      // wait for new frame
      _server->readTaskState(TaskWaitFrame);
      while (!_server->_timepix->newFrame()) {
        if (_server->_outOfOrder || _server->_shutdownFlag || !_server->_triggerConfigured) {
          goto read_shutdown;
        }
      }

      // new frame is available...
      if (buf_iter->_full) {      // is this buffer already full?
        fprintf(stderr, "Error: buffer overflow in %s\n", __PRETTY_FUNCTION__);
        // FIXME _server->setReadDamage(BufferError);
        _server->_outOfOrder = 1;
        if (_server->_occSend != NULL) {
          // send occurrence
          _server->_occSend->userMessage("Timepix: Buffer overflow in reader task\n");
        }
        goto read_shutdown;       // yes: shutdown
      } else {
        buf_iter->_full = true;   // no: mark this buffer as full
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &time1);          // TIMESTAMP 1
      }

      // read frame
      _server->readTaskState(TaskReadFrame);
      int rv = _server->_timepix->readMatrixRawPlus(buf_iter->_rawData, &ss, &lostRowBuf,
                                                    &buf_iter->_header._frameCounter,
                                                    &buf_iter->_header._timestamp);
      if (rv) {
        fprintf(stderr, "Error: readMatrixRawPlus() failed\n");
        // FIXME _server->setReadDamage(DeviceError);
      } else if ((ss != Pds::Timepix::DataV1::RawDataBytes) || (lostRowBuf != 0)) {
        fprintf(stderr, "Error: readMatrixRawPlus: sz=%u lost_rows=%d\n", ss, lostRowBuf);
      }
      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &time2);          // TIMESTAMP 2
        _profile1[buf_iter->_header.frameCounter()] = time1;
        _profile2[buf_iter->_header.frameCounter()] = time2;
      }

      // fill in lost_rows
      buf_iter->_header._lostRows = lostRowBuf;

      // send notification
      sendCommand.cmd = FrameAvailable;
      sendCommand.buf_iter = buf_iter;
      if (::write(_writeFd, &sendCommand, sizeof(sendCommand)) == -1) {
        fprintf(stderr, "%s write error: %s\n", __PRETTY_FUNCTION__, strerror(errno));
        // FIXME _server->setReadDamage(PipeError);
      }
    } // end of for
  } // end of while

read_shutdown:
  _server->readTaskState(TaskShutdown);
  printf(" ** read task shutdown **\n");
  // send shutdown command to decode task
  sendCommand.cmd = CommandShutdown;
  if (::write(_writeFd, &sendCommand, sizeof(sendCommand)) == -1) {
    fprintf(stderr, "%s write error: %s\n", __PRETTY_FUNCTION__, strerror(errno));
  }
}

void Pds::TimepixServer::DecodeRoutine::routine()
{
  command_t     receiveCommand;
  bool          missedTrigger;

  // Wait for _timepix to be set
  _server->decodeTaskState(TaskInit);
  printf(" ** decode task init **\n");
  while (_server->_timepix == NULL) {
    if (_server->_outOfOrder || _server->_shutdownFlag) {
      goto decode_shutdown;
    }
    decisleep(1);
  }
  // wait for trigger to be configured
  _server->decodeTaskState(TaskWaitConfigure);
  while (!_server->_triggerConfigured) {
    if (_server->_shutdownFlag) {
      goto decode_shutdown;
    }
    decisleep(1);
  }
  // trigger is configured
  while (1) {
    if (_server->_shutdownFlag || !_server->_triggerConfigured) {
      goto decode_shutdown;
    }
    // reset missed trigger flag
    missedTrigger = false;

    // read from pipe
    _server->decodeTaskState(TaskReadPipe);
    int length = ::read(_readFd, &receiveCommand, sizeof(receiveCommand));
    if (length != sizeof(receiveCommand)) {
      fprintf(stderr, "Error: read() returned %d in %s\n", length, __PRETTY_FUNCTION__);
      // FIXME _server->setDecodeDamage(PipeError);
    }

    if (receiveCommand.cmd == FrameAvailable) {
      vector<BufferElement>::iterator buf_iter = receiveCommand.buf_iter;

      // validate buffer
      if (!buf_iter->_full) {
        fprintf(stderr, "Error: buffer underflow in %s\n", __PRETTY_FUNCTION__);
        // FIXME _server->setDecodeDamage(BufferError);
        _server->_outOfOrder = 1;
        return;
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &_profile3[buf_iter->_header.frameCounter()]);    // TIMESTAMP 3
        _profileHwTimestamp[buf_iter->_header.frameCounter()] = buf_iter->_header._timestamp;
        _server->_profileCollected = true;
      }

      if (_server->_testData) {
        // use test image in place of real data
        memcpy(buf_iter->_pixelData, _server->_testData, Pds::Timepix::DataV1::DecodedDataBytes);
      } else if (!(_server->_debug & TIMEPIX_DEBUG_NOCONVERT)) {
        // decode to pixels
        _server->_timepix->decode2Pixels(buf_iter->_rawData, buf_iter->_pixelData);
      }

      if (_server->_debug & TIMEPIX_DEBUG_PROFILE) {
        clock_gettime(CLOCK_REALTIME, &_profile4[buf_iter->_header.frameCounter()]);    // TIMESTAMP 4
      }

    } else if (receiveCommand.cmd == CommandShutdown) {
      goto decode_shutdown;
    } else {
      fprintf(stderr, "Error: unrecognized cmd (%d) in %s\n",
              (int)receiveCommand.cmd, __PRETTY_FUNCTION__);
      continue;
    }

    // send regular payload
    _server->payloadComplete(receiveCommand.buf_iter, false);
  }

decode_shutdown:
  _server->decodeTaskState(TaskShutdown);
  printf(" ** decode task shutdown **\n");
}

unsigned Pds::TimepixServer::configure(TimepixConfigType& config)
{
  char msgBuf[80];
  // int rv;
  unsigned int configReg;
  unsigned numErrs = 0;
  int id = (int)moduleId();
  timepix_dev *tpx = (timepix_dev *)NULL;

  _count = 0;
  _countOffset = 0;
  _resetHwCount = true;
  _missedTriggerCount = 0;
  _shutdownFlag = 0;

  if (_relaxd == NULL) {
    // ---------------------------
    // Relaxd module instantiation
    // Access to a Relaxd module using an MpxModule class object:
    // parameter `id' determines IP-addr of the module: 192.168.33+id.175
    _relaxd = new MpxModule( id );

    if (verbosity() > 0) {
      // Set verbose writing to MpxModule's logfile (default = non-verbose)
      _relaxd->setLogVerbose(true);
    }
  }

  // only start receiving frames if init succeeds and sanity check passes

  if (_relaxd->init() != 0) {
    sprintf(msgBuf, "Relaxd module %d (192.168.%d.175) init failed\n", id, 33+id);
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage(msgBuf);
    }
    return (1);
  }

  if (_triggerConfigured) {
    fprintf(stderr, "Error: _triggerConfigured is true at beginning of %s\n", __PRETTY_FUNCTION__);
  }

  if (_timepix != NULL) {
    fprintf(stderr, "Error: _timepix not NULL at beginning of %s\n", __PRETTY_FUNCTION__);
  }

  int state = readTaskState();
  if (state != TaskShutdown) {
    fprintf(stderr, "Error: read task is in state %d (not shutdown) at %s line %d\n", state, __FILE__, __LINE__);
  }

  state = decodeTaskState();
  if (state != TaskShutdown) {
    fprintf(stderr, "Error: decode task is in state %d (not shutdown) at %s line %d\n", state, __FILE__, __LINE__);
  }

  // create timepix device
  tpx = new timepix_dev(id, _relaxd);

  // timepix warmup
  if (tpx->warmup(false) != 0) {
    // failed warmup
    delete tpx;
    sprintf(msgBuf, "Timepix module %d (192.168.%d.175) warmup failed\n", id, 33+id);
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage(msgBuf);
    }
    return (1);
  }

  // timepix sanity test: count the chips, disable the trigger
  int ndevs = tpx->chipCount();
  if (ndevs != TimepixConfigType::ChipCount) {
    fprintf(stderr, "Error: chipCount() returned %d (expected %d)\n",
            ndevs, TimepixConfigType::ChipCount);
    ++numErrs;
  }
  printf("Disable external trigger... ");
  _triggerConfigured = false;
  if (tpx->enableExtTrigger(false)) {
    printf("ERROR\n");
    ++numErrs;
  } else {
    printf("done\n");
  }

  // fill in configuration values which come from device

  config.chip0Name(tpx->getChipName(0).c_str());
  config.chip1Name(tpx->getChipName(1).c_str());
  config.chip2Name(tpx->getChipName(2).c_str());
  config.chip3Name(tpx->getChipName(3).c_str());

  uint32_t uu;
  if (tpx->getChipID(0, &uu) == 0) {
    config.chip0ID(uu);
  } else {
    printf("Error: failed to read Timepix chip 0 ID\n");
    ++numErrs;
  }
  if (tpx->getChipID(1, &uu) == 0) {
    config.chip1ID(uu);
  } else {
    printf("Error: failed to read Timepix chip 1 ID\n");
    ++numErrs;
  }
  if (tpx->getChipID(2, &uu) == 0) {
    config.chip2ID(uu);
  } else {
    printf("Error: failed to read Timepix chip 2 ID\n");
    ++numErrs;
  }
  if (tpx->getChipID(3, &uu) == 0) {
    config.chip3ID(uu);
  } else {
    printf("Error: failed to read Timepix chip 3 ID\n");
    ++numErrs;
  }

  config.driverVersion(tpx->getDriverVersion());

  uint32_t firm;
  if (tpx->getFirmwareVersion(&firm) == 0) {
    config.firmwareVersion(firm);
  } else {
    printf("Error: failed to read Timepix firmware version\n");
    ++numErrs;
  }

  if (numErrs > 0) {
    // failed initialization
    delete tpx;
    sprintf(msgBuf, "Timepix module %u (192.168.%d.175) init failed\n", moduleId(), 33+moduleId());
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage(msgBuf);
    }
    return (1);
  }

  // successful initialization
  // ...after _timepix is set, rely on unconfigure to delete _timepix
  _timepix = tpx;

  // call routines in separate tasks
  _readTask->call(_readRoutine);
  _decodeTask->call(_decodeRoutine);
  decisleep(1);
  state = readTaskState();
  if (state != TaskWaitConfigure) {
    fprintf(stderr, "Error: read task is in state %d (not WaitConfigure) at %s line %d\n", state, __FILE__, __LINE__);
  }
  state = decodeTaskState();
  if (state != TaskWaitConfigure) {
    fprintf(stderr, "Error: decode task is in state %d (not WaitConfigure) at %s line %d\n", state, __FILE__, __LINE__);
  }

  _readoutSpeed = config.readoutSpeed();
  _triggerMode = config.triggerMode();
  _timepixSpeed = config.timepixSpeed();

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

  // set pixels configuration

  config.pixelThresh(0, NULL);  // default: empty pixel configuration
  printf("Mode: Time Over Threshold (TOT)\n");
  if (pixelsCfg()) {
    if (_timepix->setPixelsCfg(pixelsCfg())) {
      fprintf(stderr, "Error: failed to set pixels configuraton (individual thresholds)\n");
      ++numErrs;
    } else {
      // success: store pixel configuration
      config.pixelThresh(TimepixConfigType::PixelThreshMax, pixelsCfg());
      printf("Pixels configuraton successful (individual thresholds)\n");
    }
  } else if (_timepix->setPixelsCfgTOT()) {
    fprintf(stderr, "Error: failed to set pixels configuraton (common thresholds)\n");
    ++numErrs;
  } else {
    printf("Pixels configuraton successful (common thresholds)\n");
  }

  // clear the shutter flag before setFsr()

  if (_timepix->readReg(MPIX2_CONF_REG_OFFSET, &configReg) == 0) {

     configReg &= ~(MPIX2_CONF_EXT_TRIG_ENABLE| MPIX2_CONF_TIMER_USED);

      configReg |= MPIX2_CONF_TIMER_USED;
      configReg |= MPIX2_CONF_SHUTTER_CLOSED;

    if (_timepix->writeReg(MPIX2_CONF_REG_OFFSET, configReg)) {
      fprintf(stderr, "Error: writeReg(MPIX2_CONF_REG_OFFSET) failed\n");
      ++numErrs;
    }
  } else {
    fprintf(stderr, "Error: readReg(MPIX2_CONF_REG_OFFSET) failed\n");
    ++numErrs;
  }

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

  if (_timepix->setTimepixSpeed(_timepixSpeed) ) {
    fprintf(stderr, "Error: setTimepixSpeed(%d) failed\n", _timepixSpeed);
    ++numErrs;
  }

  if (_timepix->readReg(MPIX2_CONF_REG_OFFSET, &configReg) == 0) {
    printf("Timepix configuration register: 0x%08x\n", configReg);
  }

  // ensure that pipes are empty
  // TODO drain(x); drain(y)
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
  if (_rawPipeFd[0]) {
    if (ioctl(_rawPipeFd[0], FIONREAD, &nbytes) == -1) {
      perror("ioctl");
    } else if (nbytes > 0) {
      printf("%s: draining %d bytes from raw pipe\n", __FUNCTION__, nbytes);
      while (nbytes-- > 0) {
        ::read(_rawPipeFd[0], &bucket, 1);
      }
    }
  }

  // ensure that command buffer is cleared
  vector<BufferElement>::iterator buf_iter;
  for (buf_iter = _buffer->begin(); buf_iter != _buffer->end(); buf_iter++) {
    buf_iter->_full = false;
  }

  if (numErrs > 0) {
    fprintf(stderr, "Timepix: Failed to configure.\n");
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage("Timepix: Failed to configure.\n");
    }
  } else {
    // enable polling for new frames
    printf("Enable external trigger... ");
    if (_timepix->enableExtTrigger(true)) {
      printf("ERROR\n");
      ++numErrs;
    } else {
      printf("done\n");
      // setting _triggerConfigured allows read and decode tasks to proceed
      _triggerConfigured = true;
    }
  }

  return (numErrs);
}

unsigned Pds::TimepixServer::unconfigure(void)
{
  unsigned numErrs = 0;

  _expectedDiff = 0;

  // disable polling for new frames
  if (_timepix) {
    printf("Disable external trigger... ");
    if (_timepix->enableExtTrigger(false)) {
      printf("ERROR\n");
      ++numErrs;
    } else {
      printf("done\n");
    }
  }

  _triggerConfigured = false;
  shutdown();
  // give tasks a bit of time to shutdown
  decisleep(3);

  int state = readTaskState();
  if (state != TaskShutdown) {
    fprintf(stderr, "Error: read task is in state %d (not shutdown) at %s line %d\n", state, __FILE__, __LINE__);
  } else {
    // delete timepix object
    if (_timepix) {
      delete _timepix;
      _timepix = (timepix_dev *)NULL;
    }
  }

  state = decodeTaskState();
  if (state != TaskShutdown) {
    fprintf(stderr, "Error: decode task is in state %d (not shutdown) at %s line %d\n", state, __FILE__, __LINE__);
  }

  // profiling report
  if (_profileCollected) {
    printf("Profiling Report: \n");

    long long diff1, diff2, diff3, diff4, diff5;
    diff1 = (_profile2[1].tv_nsec - _profile1[1].tv_nsec) / 1000;
    diff2 = (_profile3[1].tv_nsec - _profile2[1].tv_nsec) / 1000;
    diff3 = (_profile4[1].tv_nsec - _profile3[1].tv_nsec) / 1000;
    diff4 = (_profile5[1].tv_nsec - _profile4[1].tv_nsec) / 1000;
    diff5 = (_profile6[1].tv_nsec - _profile5[1].tv_nsec) / 1000;
    printf("Profile time 1: %ld.%09ld\n", _profile1[1].tv_sec, _profile1[1].tv_nsec);
    printf("Profile time 2: %ld.%09ld (diff %lld usec)\n", _profile2[1].tv_sec, _profile1[1].tv_nsec, diff1);
    printf("Profile time 3: %ld.%09ld (diff %lld usec)\n", _profile3[1].tv_sec, _profile2[1].tv_nsec, diff2);
    printf("Profile time 4: %ld.%09ld (diff %lld usec)\n", _profile4[1].tv_sec, _profile3[1].tv_nsec, diff3);
    printf("Profile time 5: %ld.%09ld (diff %lld usec)\n", _profile5[1].tv_sec, _profile4[1].tv_nsec, diff4);
    printf("Profile time 6: %ld.%09ld (diff %lld usec)\n", _profile6[1].tv_sec, _profile5[1].tv_nsec, diff5);
//  printf("HW Tick 1: %u\n", _profileHwTimestamp[1]);
    for (int ii = 1; ii <= 6; ii++) {
      // hw ticks are 10us
      printf("HW Tick %d: %u ", ii, _profileHwTimestamp[ii]);
      if (ii > 1) {
        printf("(diff %g msec)", (_profileHwTimestamp[ii] - _profileHwTimestamp[ii-1]) / 100.);
      }
      printf("\n");
    }
    if (_badFrame > 6) {
      printf(" ...\n");
      for (unsigned uu = _badFrame - 9 ; uu <= _badFrame; uu++) {
          printf("HW Tick %u: %u (diff %g msec)\n", uu, _profileHwTimestamp[uu],
                 (_profileHwTimestamp[uu] - _profileHwTimestamp[uu-1]) / 100.);
      }
    }
    if (_uglyFrame > 6) {
      printf(" ...\n");
      for (unsigned uu = _uglyFrame - 9 ; uu <= _uglyFrame; uu++) {
          printf("HW Tick %u: %u (diff %g msec)\n", uu, _profileHwTimestamp[uu],
                 (_profileHwTimestamp[uu] - _profileHwTimestamp[uu-1]) / 100.);
      }
    }
  }

  _count = 0;
  _missedTriggerCount = 0;
  _badFrame = 0;
  _uglyFrame = 0;

  return (numErrs);
}

unsigned Pds::TimepixServer::endrun(void)
{
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

  // read from pipe
  int length = ::read(_completedPipeFd[0], &receiveCommand, sizeof(receiveCommand));

  if (length != sizeof(receiveCommand)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", length, __FUNCTION__);
    return (-1);    // ERROR
  } else if (receiveCommand.cmd == FrameAvailable) {

    if (!receiveCommand.buf_iter->_full) {  // is this buffer empty?
      fprintf(stderr, "Error: buffer underflow in %s\n", __PRETTY_FUNCTION__);
      // latch error
      _outOfOrder = 1;
      if (_occSend) {
        // send occurrence
        _occSend->outOfOrder();
      }
      return (-1);
    }

      frame->_width = Pds::Timepix::DataV1::Width;
      frame->_height = Pds::Timepix::DataV1::Height;
      frame->_lostRows = receiveCommand.buf_iter->_header._lostRows;
      frame->_frameCounter = receiveCommand.buf_iter->_header._frameCounter;
      frame->_timestamp = receiveCommand.buf_iter->_header._timestamp;

    if (_resetHwCount) {
      _count = 0;
      _countOffset = frame->frameCounter() - 1;
      _resetHwCount = false;
    }

    ++_count;

    if (!(_debug & TIMEPIX_DEBUG_IGNORE_FRAMECOUNT)) {
      // check for out-of-order condition
      uint16_t sum16 = (uint16_t)(_count + _countOffset);
      if (frame->frameCounter() != sum16) {
        fprintf(stderr, "Error: hw framecounter (%hu) != sw count (%hu) + count offset (%u) == (%hu)\n",
                frame->frameCounter(), _count, _countOffset, sum16);
        // latch error
        _outOfOrder = 1;
        if (_occSend) {
          // send occurrence
          _occSend->outOfOrder();
        }
        return (-1);
      }
    }

    // copy xtc to payload
    if (frame->_lostRows == 0) {
      // ...undamaged
      memcpy(payload, &_xtc, sizeof(Xtc));
    } else {
      // ...damaged
      memcpy(payload, &_xtcDamaged, sizeof(Xtc));
    }

    // shuffle and copy pixels to payload
    shuffleTimepixQuad((int16_t *)frame->data(), receiveCommand.buf_iter->_pixelData);

//  memcpy((void *)frame->data(), (void *)receiveCommand.buf_iter->_pixelData,
//         frame->data_size());

    if (!receiveCommand.missedTrigger) {
      // mark buffer as empty
      receiveCommand.buf_iter->_full = false;
    }

    if (_debug & TIMEPIX_DEBUG_PROFILE) {
      // TIMESTAMP 6
      clock_gettime(CLOCK_REALTIME, &_profile6[receiveCommand.buf_iter->_header.frameCounter()]);
      // fill in timestamp 5
      _profile5[receiveCommand.buf_iter->_header.frameCounter()] = time5;
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

void TimepixServer::setOccSend(TimepixOccurrence* occSend)
{
  _occSend = occSend;
}

void TimepixServer::setTimepix(timepix_dev* timepix)
{
  _timepix = timepix;
}

Task *TimepixServer::readTask()
{
  return (_readTask);
}

Task *TimepixServer::decodeTask()
{
  return (_decodeTask);
}

void TimepixServer::shutdown()
{
  printf("\n ** TimepixServer shutdown **\n");
  _shutdownFlag = 1;
}

int TimepixServer::readTaskState(int state)
{
  _readTaskState = state;
  return (_readTaskState);
}

int TimepixServer::readTaskState()
{
  return (_readTaskState);
}

int TimepixServer::decodeTaskState(int state)
{
  _decodeTaskState = state;
  return (_decodeTaskState);
}

int TimepixServer::decodeTaskState()
{
  return (_decodeTaskState);
}

static void shuffleTimepixQuad(int16_t *dst, int16_t *src)
{
  unsigned destX, destY;
  for(unsigned iy=0; iy<2*512; iy++) {
    for(unsigned k=0; k<512/2; k++, src++) {
      // map pixels from 256x1024 to 512x512
      switch (iy / 256) {
        case 0:
          destX = iy;
          destY = 511 - k;
          break;
        case 1:
          destX = iy - 256;
          destY = 255 - k;
          break;
        case 2:
          destX = 1023 - iy;
          destY = k;
          break;
        case 3:
          destX = 1023 + 256 - iy;
          destY = k + 256;
          break;
        default:
          // error
          destX = destY = 0;  // suppress warning
          break;
      }
      dst[destX + (destY * 512)] = *src;
    }
  }
}
