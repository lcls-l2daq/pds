// timepix_dev.hh
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __TIMEPIX_DEV_HH
#define __TIMEPIX_DEV_HH

#include <unistd.h>

#include "pdsdata/timepix/ConfigV1.hh"
#include "pds/service/Semaphore.hh"
#include "mpxmodule.h"

namespace Pds
{
  class timepix_dev;
}

class Pds::timepix_dev {
public:
  timepix_dev(int moduleId, MpxModule *relaxd);
  ~timepix_dev() {}

  // relaxd wrapper functions
  int chipCount();
  uint32_t lastClockTick();
  uint16_t lastFrameCount();
  int resetFrameCounter();
  bool newFrame(bool check=true, bool clear=true);
  int readMatrixRaw(uint8_t *bytes, uint32_t *sz, int *lost_rows);
  int readReg(uint32_t offset, uint32_t *value);
  int writeReg(uint32_t offset, uint32_t value);
  int setFsr(int chipnr, int *dac, uint32_t col_testpulse_reg=0);

private:
  int          _moduleId;
  MpxModule*   _relaxd;
  Semaphore*   _mutex;
};
  
#endif
