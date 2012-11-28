// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "timepix_dev.hh"

using namespace Pds;

timepix_dev::timepix_dev(int moduleId, MpxModule *relaxd) :
  _moduleId(moduleId),
  _relaxd(relaxd),
  _adjustFrameCount(0),
  _localCount(0),
  _frameCountOffset(0),
  _firstRead(true)
{
  _mutex = new Semaphore(Semaphore::FULL);
}

timepix_dev::~timepix_dev()
{
  delete _mutex;
}

// relaxd wrapper functions

// Using a semaphore, we ensure that no two threads enter
// the relaxd library at the same time.

int timepix_dev::chipCount()
{
  int rv;

  _mutex->take();
  rv = _relaxd->chipCount();
  _mutex->give();

  return (rv);
}

bool timepix_dev::newFrame(bool check, bool clear)
{
  bool rv;

  _mutex->take();
  rv = _relaxd->newFrame(check, clear);
  _mutex->give();

  return (rv);
}

int timepix_dev::readMatrixRaw(uint8_t *bytes, uint32_t *sz, int *lost_rows)
{
  int rv;

  _mutex->take();
  rv = _relaxd->readMatrixRaw(bytes, sz, lost_rows);
  _mutex->give();

  return (rv);
}

int timepix_dev::setPixelsCfgTOT(void)
{
  int rv2, rv3;

  _mutex->take();

  // this is required before setPixelsCfg()
  _relaxd->setParReadout(true);

  rv2 = _relaxd->setPixelsCfg(TPX_MODE_TOT, 0, 0);

  // this is required after every call to setPixelsCfg()
  rv3 = _relaxd->resetMatrix();

  _mutex->give();

  return (rv2+rv3);
}

int timepix_dev::readMatrixRawPlus(uint8_t *bytes, uint32_t *sz, int *lost_rows,
                                   uint16_t *lastFrameCount, uint32_t *lastClockTick)
{
  int rv;
  uint16_t hwFrameCount;

  _mutex->take();
  // read raw data
  rv = _relaxd->readMatrixRaw(bytes, sz, lost_rows);

  // read frame counter
  hwFrameCount = _relaxd->lastFrameCount();

  // adjust frame counter
  if (_firstRead) {
    _localCount = hwFrameCount;
    _frameCountOffset = 0;
    _firstRead = false;
  } else {
    ++ _localCount;
  }

  int off = _localCount - (uint16_t)(hwFrameCount + _frameCountOffset);

  if (off == 1) {
    // workaround for repeating frame counter behavior
    ++_frameCountOffset;
  }

  if (lastFrameCount) {
    *lastFrameCount = hwFrameCount + _frameCountOffset;
  }

  // read timestamp
  if (lastClockTick) {
    *lastClockTick = _relaxd->lastClockTick();
  }
  _mutex->give();

  return (rv);
}

int timepix_dev::readReg(uint32_t offset, uint32_t *value)
{
  int rv;

  _mutex->take();
  rv = _relaxd->readReg(offset, value);
  _mutex->give();

  return (rv);
}

int timepix_dev::writeReg(uint32_t offset, uint32_t value)
{
  int rv;

  _mutex->take();
  rv = _relaxd->writeReg(offset, value);
  _mutex->give();

  return (rv);
}

int timepix_dev::setFsr(int chipnr, int *dac, uint32_t col_testpulse_reg)
{
  int rv;

  _mutex->take();
  rv = _relaxd->setFsr(chipnr, dac, col_testpulse_reg);
  _mutex->give();

  return (rv);
}

void timepix_dev::decode2Pixels(uint8_t *bytes, int16_t *pixels)
{
  _relaxd->decode2Pixels(bytes, pixels);  // no mutex needed
}

int timepix_dev::enableExtTrigger(bool enable)
{
  int rv;

  _mutex->take();
  rv = _relaxd->enableExtTrigger(enable);
  _mutex->give();

  return (rv);
}

int timepix_dev::setPixelsCfg(uint8_t *bytes)
{
  int rv1, rv2;

  _mutex->take();
  rv1 = _relaxd->setPixelsCfg(bytes);

  // this is required after every call to setPixelsCfg()
  rv2 = _relaxd->resetMatrix();
  _mutex->give();

  return rv1+rv2;
}

int timepix_dev::setTimepixSpeed(int32_t speed)
{
  int rv = 1;

  if (speed >=0 && speed <= 4) {
    _mutex->take();
    _relaxd->rmwReg( MPIX2_CONF_REG_OFFSET,
                 speed << MPIX2_CONF_TPX_CLOCK_SHIFT,
                 MPIX2_CONF_TPX_CLOCK_MASK );
    _mutex->give();
    rv = 0; // return success
  }

  return rv;
}

int timepix_dev::getDriverVersion()
{
  _mutex->take();
  int rv = _relaxd->getDriverVersion();
  _mutex->give();
  return rv;
}

int timepix_dev::getFirmwareVersion(uint32_t *ver)
{
  _mutex->take();
  int rv = _relaxd->getFirmwareVersion(ver);
  _mutex->give();
  return rv;
}

std::string timepix_dev::getChipName(int chipnr)
{

  _mutex->take();
  std::string buf = _relaxd->chipName(chipnr);
  _mutex->give();
  
  return buf;
}

int timepix_dev::getChipID(int chipnr, uint32_t *id)
{
  _mutex->take();
  int rv = _relaxd->readChipId(chipnr, id);
  _mutex->give();
  return rv;
}

int timepix_dev::readFirmware(int page, uint8_t *bytes)
{
  int rv = 1;

  if (page >=0 && page < 4096 && bytes != NULL) {
    _mutex->take();
    rv = _relaxd->readFirmware(page, bytes);
    _mutex->give();
  }

  return rv;
}

//
// warmup - Timepix initialization required for reliable startup
//
int timepix_dev::warmup(bool init)
{
  unsigned int confReg;
  char *       errString = (char *)NULL;
  uint8_t *    warmupbuf = new uint8_t[500000]; // readMatrixRaw()

  printf("Timepix warmup... ");

  _mutex->take();

  if (init && _relaxd->init()) {
    errString = "relaxd init() failed";
    goto warmup_error;
  }

  if (_relaxd->readReg(MPIX2_CONF_REG_OFFSET, &confReg)) {
    errString = "readReg(MPIX2_CONF_REG_OFFSET) failed";
    goto warmup_error;
  }

  // ASI - TPX clock frequencies on the new board are
  // 10, 50 and 100 MHz, so no 2.5MHz anymore (Nov 2012)

  // set low clock frequency (10 MHz)
  confReg &= ~MPIX2_CONF_TPX_CLOCK_MASK;
  confReg |= MPIX2_CONF_TPX_CLOCK_10MHZ;

  if (_relaxd->writeReg(MPIX2_CONF_REG_OFFSET, confReg)) {
    errString = "writeReg(MPIX2_CONF_REG_OFFSET) failed";
    goto warmup_error;
  }

  // Use the shutter timer, at 100 microseconds
  // (note that the Relaxd timer has a resolution of 10 us)
  // ### Due to a bug(?) in Relaxd we need to repeat enableTimer()
  //     after each call to setDacs() (renamed setFsr())
  _relaxd->enableTimer(true, 100);

  // do a couple of frame read-outs with a delay (1 sec)
  unsigned int  sz;
  int           lost_rows;

  // Open the shutter: starts the timer
  _relaxd->openShutter();

  // Wait for the Relaxd timer to close the shutter
  while( !_relaxd->timerExpired() ) {
    usleep(10);
  }

  // readMatrixRaw: sz=458752  lost_rows=0
  if (_relaxd->readMatrixRaw(warmupbuf, &sz, &lost_rows) ) {
    errString = "readMatrixRaw() failed";
    goto warmup_error;
  }

  sleep(1);   // delay 1 sec

  // Open the shutter: starts the timer
  _relaxd->openShutter();

  // Wait for the Relaxd timer to close the shutter
  while( !_relaxd->timerExpired() ) {
    usleep(10);
  }

  // readMatrixRaw: sz=458752  lost_rows=0
  if (_relaxd->readMatrixRaw(warmupbuf, &sz, &lost_rows) ) {
    errString = "readMatrixRaw() failed";
    goto warmup_error;
  }

  if (_relaxd->readReg(MPIX2_CONF_REG_OFFSET, &confReg)) {
    errString = "readReg(MPIX2_CONF_REG_OFFSET) failed";
    goto warmup_error;
  }

  // set high clock frequency (100 MHz)
  confReg &= ~MPIX2_CONF_TPX_CLOCK_MASK;
  confReg |= MPIX2_CONF_TPX_CLOCK_100MHZ;

  if (_relaxd->writeReg(MPIX2_CONF_REG_OFFSET, confReg)) {
    errString = "writeReg(MPIX2_CONF_REG_OFFSET) failed";
    goto warmup_error;
  }

  // Successful return
  _mutex->give();
  delete[] warmupbuf;
  printf("done\n");
  return (0);

warmup_error:
  // ERROR return
  _mutex->give();
  delete[] warmupbuf;
  if (errString) {
    printf("ERROR: %s\n", errString);
  }
  return (1);
}
