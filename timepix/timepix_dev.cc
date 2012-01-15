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
