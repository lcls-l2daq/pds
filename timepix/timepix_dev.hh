// $Id$
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
  ~timepix_dev();

  // relaxd wrapper functions
  int chipCount();
  bool newFrame(bool check=true, bool clear=true);
  int readMatrixRaw(uint8_t *bytes, uint32_t *sz, int *lost_rows);
  int readMatrixRawPlus(uint8_t *bytes, uint32_t *sz, int *lost_rows,
                        uint16_t *lastFrameCount, uint32_t *lastClockTick);
  int readReg(uint32_t offset, uint32_t *value);
  int writeReg(uint32_t offset, uint32_t value);
  int setFsr(int chipnr, int *dac, uint32_t col_testpulse_reg=0);
  void decode2Pixels(uint8_t *bytes, int16_t *pixels);
  int enableExtTrigger(bool enable);
  int setPixelsCfgTOT(void);
  int setPixelsCfg(uint8_t *bytes);
  int setTimepixSpeed(int32_t speed);
  int getDriverVersion();
  int getFirmwareVersion(uint32_t *ver);
  std::string getChipName(int chipnr);
  int getChipID(int chipnr, uint32_t *id);
  int readFirmware(int page, uint8_t *bytes);
  // Dec 2012 extension
  int hwInfoCount();
  int getHwInfoFlags(int index, u32 *flags);
  int getHwInfo(int index, HwInfoItem *hw_item, int *sz);
  int setHwInfo(int index, void *data, int sz);

  // Timepix warmup
  int warmup(bool init);

private:
  int          _moduleId;
  MpxModule*   _relaxd;
  Semaphore*   _mutex;
  uint16_t    _adjustFrameCount;
  uint16_t    _localCount;
  int         _frameCountOffset;
  bool        _firstRead;
};
  
#endif
