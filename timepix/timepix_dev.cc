// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "timepix_dev.hh"

using namespace Pds;

timepix_dev::timepix_dev(int moduleId, MpxModule *relaxd) :
  _moduleId(moduleId),
  _relaxd(relaxd)
{
  _mutex = new Semaphore(Semaphore::FULL);
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

uint32_t timepix_dev::lastClockTick()
{
  uint32_t rv;

  _mutex->take();
  rv = _relaxd->lastClockTick();
  _mutex->give();

  return (rv);
}

uint16_t timepix_dev::lastFrameCount()
{
  uint16_t rv;

  _mutex->take();
  rv = _relaxd->lastFrameCount();
  _mutex->give();

  return (rv);
}

int timepix_dev::resetFrameCounter()
{
  int rv;

  _mutex->take();
  rv = _relaxd->resetFrameCounter();
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

int timepix_dev::readMatrixRawPlus(uint8_t *bytes, uint32_t *sz, int *lost_rows,
                                   uint16_t *lastFrameCount, uint32_t *lastClockTick)
{
  int rv;

  _mutex->take();
  // read raw data
  rv = _relaxd->readMatrixRaw(bytes, sz, lost_rows);

  // read frame counter
  if (lastFrameCount) {
    *lastFrameCount = _relaxd->lastFrameCount();
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

//
// warmup - Timepix initialization required for reliable startup
//
int timepix_dev::warmup(bool init)
{
  unsigned int confReg;
  char *       errString = (char *)NULL;
  uint8_t *    warmupbuf = new uint8_t[500000]; // readMatrixRaw()

  printf("Starting Timepix warmup...\n");

  _mutex->take();

  if (init && _relaxd->init()) {
    errString = "relaxd init() failed";
    goto warmup_error;
  }

  if (_relaxd->readReg(MPIX2_CONF_REG_OFFSET, &confReg)) {
    errString = "readReg(MPIX2_CONF_REG_OFFSET) failed";
    goto warmup_error;
  }

  // set low clock frequency (2.5 MHz)
  confReg &= ~MPIX2_CONF_TPX_CLOCK_MASK;
  confReg |= MPIX2_CONF_TPX_CLOCK_2MHZ5;

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
  printf("...Timepix warmup done\n");
  return (0);

warmup_error:
  // ERROR return
  _mutex->give();
  delete[] warmupbuf;
  if (errString) {
    printf("...Timepix warmup error: %s\n", errString);
  }
  return (1);
}
