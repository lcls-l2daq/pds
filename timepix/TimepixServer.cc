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
static int set_cpu_affinity(int cpu_id);

Pds::TimepixServer::TimepixServer( const Src& client, unsigned moduleId, unsigned verbosity, unsigned debug, char *threshFile,
                                   char *testImageFile, int cpu0, int cpu1)
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
     _shutdownFlag(0),
     _pixelsCfg(NULL),
     _threshFile(threshFile),
     _testData(NULL),
     _relaxd(NULL),
     _cpu0(cpu0),
     _cpu1(cpu1),
     _readTaskMutex(new Semaphore(Semaphore::FULL)),
     _threshFileError(false)
{
  // allocate read tasks and buffers
  for (int ii = 0; ii < ReadThreads; ii++) {
    _readTask[ii] = new Task(TaskObject("read"));
    _buffer[ii] = new vector<BufferElement>(BufferDepth);
  }

  // calculate aux data xtc extent
  _xtc.extent = sizeof(TimepixDataType) + sizeof(Xtc) + Pds::TimepixData::DecodedDataBytes;

  // alternate aux data xtc is used for damaged events
  _xtcDamaged.extent = sizeof(TimepixDataType) + sizeof(Xtc) + Pds::TimepixData::DecodedDataBytes;
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

  // create completed pipe
  int err = ::pipe(_completedPipeFd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_completedPipeFd[0]);
  }

  // create routines (but do not yet call them)
  _readRoutine[0] = new ReadRoutine(this, 0, _cpu0);
  _readRoutine[1] = new ReadRoutine(this, 1, _cpu1);

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
      perror("TimepixServer: fopen bpc");
      _threshFileError = true;
    } else {
      _pixelsCfg = new uint8_t[TimepixConfigType::PixelThreshMax];
      if (fread(_pixelsCfg, TimepixConfigType::PixelThreshMax, 1, fp) != 1) {
        perror("TimepixServer: fread bpc");
        _threshFileError = true;
        delete[] _pixelsCfg;
        _pixelsCfg = NULL;
      }
      // the Timepix mode (TOT or Counting) will be set later, after configuration
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
  if (_pixelsCfg) {
    for (int jj=0; jj < TimepixConfigType::PixelThreshMax; jj++) {
      // be sure to read the configuration before setting the mode
      _pixelsCfg[jj] &= ~TPX_CFG8_MODE_MASK;
      if (_timepixMode) {
        // TOT mode
        _pixelsCfg[jj] |= (TPX_MODE_TOT << TPX_CFG8_MODE_MASK_SHIFT);
      } else {
        // Counting mode
        _pixelsCfg[jj] |= (TPX_MODE_MEDIPIX << TPX_CFG8_MODE_MASK_SHIFT);
      }
    }
  }
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
  uint32_t timestamp;
  uint16_t frameCounter;
  int lostRowBuf;
  vector<BufferElement>::iterator buf_iter;
  command_t sendCommand;

  if ((_taskNum < 0) || (_taskNum >= ReadThreads)) {
    printf("Error: _taskNum = %d in %s\n", _taskNum, __PRETTY_FUNCTION__);
    return;
  }

  vector<BufferElement>* buffer = _server->_buffer[_taskNum];

  if ((_cpuAffinity >= 0) && (set_cpu_affinity(_cpuAffinity) != 0)) {
      printf(" ** read task %d set_cpu_affinity(%d) failed **\n",
             _taskNum, _cpuAffinity);
  }

  // Wait for _timepix to be set
  printf(" ** read task %d init **\n", _taskNum);
  while (_server->_timepix == NULL) {
    if (_server->_outOfOrder || _server->_shutdownFlag) {
      goto read_shutdown;
    }
    decisleep(1);
  }

  // wait for trigger to be configured
  while (!_server->_triggerConfigured) {
    decisleep(1);
    if (_server->_outOfOrder || _server->_shutdownFlag) {
      goto read_shutdown;
    }
  }

  printf(" ** read task %d waiting for frame **\n", _taskNum);

  while (1) {
    for (buf_iter = buffer->begin(); buf_iter != buffer->end(); buf_iter++) {
      // ------------ BEGIN CRITICAL SECTION -------------------
      // take semaphore to protect critical section (wait...read)
      _server->_readTaskMutex->take();

      // wait for new frame
      while (!_server->_timepix->newFrame()) {
        if (_server->_outOfOrder || _server->_shutdownFlag || !_server->_triggerConfigured) {
          // give semaphore
          _server->_readTaskMutex->give();
          goto read_shutdown;
        }
      }

      // new frame is available...
      if (buf_iter->_full) {      // is this buffer already full?
        // give semaphore
        _server->_readTaskMutex->give();
        fprintf(stderr, "Error: buffer overflow in %s\n", __PRETTY_FUNCTION__);
        _server->_outOfOrder = 1;
        if (_server->_occSend != NULL) {
          // send occurrence
          _server->_occSend->userMessage("Timepix: Buffer overflow in reader task\n");
        }
        goto read_shutdown;       // yes: shutdown
      } else {
        buf_iter->_full = true;   // no: mark this buffer as full
      }

      // read frame
      int rv = _server->_timepix->readMatrixRawPlus(buf_iter->_rawData, &ss, &lostRowBuf,
                                                    &frameCounter, &timestamp);
      // give semaphore
      _server->_readTaskMutex->give();
      // ------------ END CRITICAL SECTION -------------------

      if (rv) {
        fprintf(stderr, "Error: readMatrixRawPlus() failed (read task %d)\n", _taskNum);
        // FIXME _server->setReadDamage(DeviceError);
      } else if ((ss != Pds::TimepixData::RawDataBytes) || (lostRowBuf != 0)) {
        fprintf(stderr, "Error: readMatrixRawPlus: sz=%u lost_rows=%d (read task %d)\n",
                ss, lostRowBuf, _taskNum);
      }

      // fill in header
      *new(&buf_iter->_header) Pds::Timepix::DataV1(timestamp, frameCounter, lostRowBuf);

      // decode frame
      if (_server->_testData) {
        // use test image in place of real data
        memcpy(buf_iter->_pixelData, _server->_testData, Pds::TimepixData::DecodedDataBytes);
      } else if (!(_server->_debug & TIMEPIX_DEBUG_NOCONVERT)) {
        // decode to pixels
        _server->_timepix->decode2Pixels(buf_iter->_rawData, buf_iter->_pixelData);
      }

      // send regular payload
      _server->payloadComplete(buf_iter, false);

    } // end of for
  } // end of while

read_shutdown:
  printf(" ** read task %d shutdown **\n", _taskNum);
}

unsigned Pds::TimepixServer::configure(TimepixConfigType& config)
{
  char msgBuf[100];
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

  if (_threshFileError) {
    snprintf(msgBuf, sizeof(msgBuf), "Timepix: Error reading bpc file '%s'\n", _threshFile);
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      // send occurrence
      _occSend->userMessage(msgBuf);
    }
    return (1);
  }

  // only start receiving frames if init succeeds and sanity check passes

  if (_relaxd->init() != 0) {
    snprintf(msgBuf, sizeof(msgBuf), "Timepix: Relaxd module %d (192.168.%d.175) init failed\n", id, 33+id);
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

  // create timepix device
  tpx = new timepix_dev(id, _relaxd);

  // timepix warmup
  if (tpx->warmup(false) != 0) {
    // failed warmup
    delete tpx;
    snprintf(msgBuf, sizeof(msgBuf), "Timepix module %d (192.168.%d.175) warmup failed\n", id, 33+id);
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
  ndarray<std::string,1> chipNames = make_ndarray<std::string>(4);
  ndarray<uint32_t   ,1> chipIDs   = make_ndarray<uint32_t   >(4);
  int32_t  driverVersion;
  uint32_t firm;

  for(unsigned i=0; i<4; i++)
    chipNames[i] = tpx->getChipName(i);

  for(unsigned i=0; i<4; i++)
    if (tpx->getChipID(0, &chipIDs[i]) != 0) {
      printf("Error: failed to read Timepix chip %d ID\n",i);
      ++numErrs;
    }

  driverVersion = tpx->getDriverVersion();

  if (tpx->getFirmwareVersion(&firm) != 0) {
    printf("Error: failed to read Timepix firmware version\n");
    ++numErrs;
  }

  if (numErrs > 0) {
    // failed initialization
    delete tpx;
    snprintf(msgBuf, sizeof(msgBuf), "Timepix module %u (192.168.%d.175) init failed\n", moduleId(), 33+moduleId());
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

  // create reader threads
  for (int ii = 0; ii < ReadThreads; ii++) {
    _readTask[ii]->call(_readRoutine[ii]);
  }

  decisleep(1);

  _readoutSpeed = config.readoutSpeed();
  _timepixMode = config.timepixMode();
  _timepixSpeed = config.timepixSpeed();
  _dacBias = config.dacBias();

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

  if (_timepixMode) {
    printf("Timepix Mode: Time Over Threshold (TOT)\n");
  } else {
    printf("Timepix Mode: Counting\n");
  }
  if (pixelsCfg()) {
    if (_timepix->setPixelsCfg(pixelsCfg())) {
      fprintf(stderr, "Error: failed to set pixels configuraton (individual thresholds)\n");
      ++numErrs;
    } else {
      // success: store pixel configuration
      Pds::TimepixConfig::setConfig(config, chipNames, chipIDs, driverVersion, firm, 
                                    TimepixConfigType::PixelThreshMax, pixelsCfg());
      printf("Pixels configuraton successful (individual thresholds)\n");
    }
  } else if (_timepix->setPixelsCfgTOT()) {
    fprintf(stderr, "Error: failed to set pixels configuraton (common thresholds)\n");
    ++numErrs;
  } else {
    Pds::TimepixConfig::setConfig(config, chipNames, chipIDs, driverVersion, firm);
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

  uint32_t pct = ((uint32_t)_dacBias - 50) * 100 / 50; // convert 50-100V to 0-100%
  if (_timepix->setHwInfo(HW_ITEM_BIAS_VOLTAGE_ADJUST, (void *)&pct, sizeof(pct))) {
    fprintf(stderr, "Error: setHwInfo(HW_ITEM_BIAS_VOLTAGE_ADJUST, %u) failed\n", pct);
    ++numErrs;
  }

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

  // ensure that command buffer is cleared
  vector<BufferElement>::iterator buf_iter;
  for (int ii = 0; ii < ReadThreads; ii++) {
    for (buf_iter = _buffer[ii]->begin(); buf_iter != _buffer[ii]->end(); buf_iter++) {
      buf_iter->_full = false;
    }
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
      // setting _triggerConfigured allows read tasks to proceed
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

  // FIXME confirm that tasks are shutdown before delete

  // delete timepix object
  if (_timepix) {
    delete _timepix;
    _timepix = (timepix_dev *)NULL;
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
  char msgBuf[80];

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
      snprintf(msgBuf, sizeof(msgBuf), "Timepix buffer underflow\n");
      fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
      // latch error
      _outOfOrder = 1;
      if (_occSend) {
        // send occurrence
        _occSend->userMessage(msgBuf);
        _occSend->outOfOrder();
      }
      return (-1);
    }

    *new(frame) TimepixDataType(Pds::Timepix::DataV1::Width,
                                Pds::Timepix::DataV1::Height,
                                receiveCommand.buf_iter->_header.timestamp(),
                                receiveCommand.buf_iter->_header.frameCounter(),
                                receiveCommand.buf_iter->_header.lostRows());

    if (_resetHwCount) {
      _count = 0;
      _countOffset = frame->frameCounter() - 1;
      _resetHwCount = false;
    }

    ++_count;
    uint16_t sum16 = (uint16_t)(_count + _countOffset);

    if (!(_debug & TIMEPIX_DEBUG_IGNORE_FRAMECOUNT)) {
      // check for out-of-order condition
      if (frame->frameCounter() != sum16) {
        _badFrame = sum16;
        snprintf(msgBuf, sizeof(msgBuf), "Timepix: hw count (%hu) != sw count (%hu) + offset (%u) == (%hu)\n",
                frame->frameCounter(), _count, _countOffset, sum16);
        fprintf(stderr, "%s: %s", __FUNCTION__, msgBuf);
        // latch error
        _outOfOrder = 1;
        if (_occSend) {
          // send occurrence
          _occSend->userMessage(msgBuf);
          _occSend->outOfOrder();
        }
        return (-1);
      }
    }

    // copy xtc to payload
    if (frame->lostRows() == 0) {
      // ...undamaged
      memcpy(payload, &_xtc, sizeof(Xtc));
    } else {
      // ...damaged
      memcpy(payload, &_xtcDamaged, sizeof(Xtc));
    }

    // shuffle and copy pixels to payload
    shuffleTimepixQuad((int16_t *)frame->data().data(), receiveCommand.buf_iter->_pixelData);

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

void TimepixServer::shutdown()
{
  printf("\n ** TimepixServer shutdown **\n");
  _shutdownFlag = 1;
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

