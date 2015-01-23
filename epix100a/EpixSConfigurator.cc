/*
 * EpixSConfigurator.cc
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Configurator.hh"
#include "pds/epixS/EpixSConfigurator.hh"
#include "pds/epixS/EpixSDestination.hh"
#include "pds/epixS/EpixSStatusRegisters.hh"
#include "ndarray/ndarray.h"

using namespace Pds::EpixS;

class EpixSDestination;

//  configAddrs array elements are address:useModes

static uint32_t configAddrs[EpixSConfigShadow::NumberOfValues][2] = {
    {0x00,  1}, //  version
    {0x02,  0}, //  RunTrigDelay
    {0x04,  2}, //  DaqTrigDelay
    {0x07,  0}, //  DacSetting
    {0x29,  0}, //  AsicPins, etc
    {0x2a,  0}, //  AsicPinControl, etc
    {0x20,  0}, //  AcqToAsicR0Delay
    {0x21,  0}, //  AsicR0ToAsicAcq
    {0x22,  0}, //  AsicAcqWidth
    {0x23,  0}, //  AsicAcqLToPPmatL
    {0x3a,  0}, //  AsicPPmatToReadout
    {0x24,  0}, //  AsicRoClkHalfT
    {0x25,  0}, //  AdcReadsPerPixel
    {0x26,  0}, //  AdcClkHalfT
    {0x2b,  0}, //  AsicR0Width
    {0x2c,  0}, //  AdcPipelineDelay
    {0x2d,  0}, //  SyncParams
    {0x2e,  0}, //  PrepulseR0Width
    {0x2f,  0}, //  PrepulseR0Delay
    {0x30,  1}, //  DigitalCardId0
    {0x31,  1}, //  DigitalCardId1
    {0x32,  1}, //  AnalogCardId0
    {0x33,  1}, //  AnalogCardId1
    {0x3b,  1}, //  CarrierId0
    {0x3c,  1}, //  CarrierId1
    {0,     2}, //  NumberOfAsicsPerRow
    {0,     2}, //  NumberOfAsicsPerColumn
    {0,     2}, //  NumberOfRowsPerAsic
    {0,     2}, //  NumberOfReadableRowsPerAsic
    {0,     2}, //  NumberOfPixelsPerAsicRow
    {0,     2}, // CalibrationRowCountPerASIC,
    {0,     2}, // EnvironmentalRowCountPerASIC,
    {0x10,  1}, //  BaseClockFrequency
    {0xd,   0}, //  AsicMask
    {0x52,  0}, //  ScopeSetup1
    {0x53,  0}, //  ScopeSetup2
    {0x54,  0}, //  ScopeLengthAndSkip
    {0x55,  0}, //  ScopeInputSelects
};

static uint32_t AconfigAddrs[EpixSASIC_ConfigShadow::NumberOfValues][2] = {
    {0x1001,  0},  //  pulserVsPixelOnDelay and pulserSync
    {0x1002,  0},  //  dummy pixel
    {0x1003,  0},  //
    {0x1004,  0},  //
    {0x1005,  0},  //
    {0x1006,  0},  //
    {0x1007,  0},  //
    {0x1008,  0},  //
    {0x1009,  0},  //
    {0x100a,  0},  //
    {0x100b,  0},  //
    {0x100c,  0},  //
    {0x100d,  0},  //
    {0x100e,  0},  //  
    {0x100f,  0},  //  
    {0x1010,  0},  //  
    {0x1011,  3},  //  RowStartAddr
    {0x1012,  3},  //  RowStopAddr
    {0x1013,  3},  //  ColStartAddr
    {0x1014,  3},  //  ColStopAddr
    {0x1015,  1}  //  chipID
};


EpixSConfigurator::EpixSConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _testModeState(0), _config(0), _s(0), _rhisto(0),
                          _maintainLostRunTrigger(0) {
  printf("EpixSConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

EpixSConfigurator::~EpixSConfigurator() {}

void EpixSConfigurator::resetFrontEnd() {
  _d.dest(EpixSDestination::Registers);
  _pgp->writeRegister(&_d, ResetAddr, 1);
  microSpin(10);
}

void EpixSConfigurator::resetSequenceCount() {
  _d.dest(EpixSDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, ResetFrameCounter, 1);
    _pgp->writeRegister(&_d, ResetAcqCounter, 1);
    microSpin(10);
  } else printf("EpixSConfigurator::resetSequenceCount() found nil _pgp so not reset\n");
}

uint32_t EpixSConfigurator::sequenceCount() {
  _d.dest(EpixSDestination::Registers);
  uint32_t count=1111;
  if (_pgp) _pgp->readRegister(&_d, ReadFrameCounter, 0x5e4, &count);
  else printf("EpixSConfigurator::sequenceCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t EpixSConfigurator::acquisitionCount() {
  _d.dest(EpixSDestination::Registers);
  uint32_t count=1112;
  if (_pgp) _pgp->readRegister(&_d, ReadAcqCounter, 0x5e4, &count);
  else printf("EpixSConfigurator::acquisitionCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t EpixSConfigurator::lowerCarrierId() {
  _d.dest(EpixSDestination::Registers);
  uint32_t count=1113;
  if (_pgp) _pgp->readRegister(&_d, LowerCarrierIdAddr, 0x5e4, &count);
  else printf("EpixSConfigurator::lowerCarrierId() found nil _pgp so not read\n");
  return (count);
}

uint32_t EpixSConfigurator::upperCarrierId() {
  _d.dest(EpixSDestination::Registers);
  uint32_t count=1114;
  if (_pgp) _pgp->readRegister(&_d, UpperCarrierIdAddr, 0x5e4, &count);
  else printf("EpixSConfigurator::upperCarrierId() found nil _pgp so not read\n");
  return (count);
}

void EpixSConfigurator::enableExternalTrigger(bool f) {
  _d.dest(EpixSDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, DaqTriggerEnable, f ? Enable : Disable);
  } else printf("EpixSConfigurator::enableExternalTrigger(%s) found nil _pgp so not set\n", f ? "true" : "false");
}

void EpixSConfigurator::enableRunTrigger(bool f) {
  _d.dest(EpixSDestination::Registers);
  _pgp->writeRegister(&_d, RunTriggerEnable, f ? Enable : Disable);
}

void EpixSConfigurator::print() {}

void EpixSConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

void EpixSConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("EpixSConfigurator::runTimeConfigName(%s)\n", name);
}

bool EpixSConfigurator::_flush(unsigned index) {
  enum {numberOfTries=50};
  unsigned version = 0;
  unsigned failCount = 0;
  bool ret = false;
  printf("\n\t--flush-%u-", index);
  while ((failCount++<numberOfTries)
      && (Failure == _pgp->readRegister(&_d, VersionAddr, 0x5e4, &version))) {
    printf("%s(%u)-", _d.name(), failCount);
  }
  if (failCount<numberOfTries) {
    printf("%s version(0x%x)\n\t", _d.name(), version);
  }
  else {
    ret = true;
    printf("_flush FAILED!!\n\t");
  }
  return ret;
}

unsigned EpixSConfigurator::configure( EpixSConfigType* c, unsigned first) {
  _config = c;
  _s = (EpixSConfigShadow*) c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  bool printFlag = true;
  if (printFlag) printf("EpixS Config size(%u)", c->_sizeof());
  printf(" config(%p) first(%u)\n", _config, first);
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  printf("EpixSConfigurator::configure %sreseting front end\n", first ? "" : "not ");
  if (first) {
    resetFrontEnd();
    printf("\tSleeping 9 seconds\n");
    sleep(9);
  }
  if (_flush()) {
    printf("EpixSConfigurator::configure determined that we lost contact with the front end, exiting!\n");
    return 1;
  }
  resetSequenceCount();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  if (printFlag) printf("\n\twriting top level config");
  enableRunTrigger(false);
  ret |= writeConfig();
  enableRunTrigger(true);
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  ret <<= 1;
  if (printFlag) printf("\n\twriting ASIC regs");
  enableRunTrigger(false);
  ret |= writeASIC();
  loadRunTimeConfigAdditions(_runTimeConfigFileName);
  enableRunTrigger(true);
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  ret <<= 1;
  if (usleep(10000)<0) perror("EpixSConfigurator::configure second ulseep failed\n");
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone \n", ret);
    printf(" it took %lld.%lld milliseconds with first %u\n", diff/1000000LL, diff%1000000LL, first);
    if (ret) dumpFrontEnd();
  }
  return ret;
}

static uint32_t FPGAregs[6][3] = {
    {PowerEnableAddr, PowerEnableValue, 0},
    {SaciClkBitAddr, SaciClkBitValue, 0},
    {NumberClockTicksPerRunTriggerAddr, NumberClockTicksPerRunTrigger, 0},  // remove when in config
    {EnableAutomaticRunTriggerAddr, 0, 0},  // remove when in config
    {EnableAutomaticDaqTriggerAddr, 0, 0},
    {0,0,1}
};

unsigned EpixSConfigurator::writeConfig() {
  _d.dest(EpixSDestination::Registers);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  unsigned i=0;
  printf("\n");
  while ((ret == Success) && (FPGAregs[i][2] == 0)) {
    if (_debug & 1) printf("EpixSConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", FPGAregs[i][0], FPGAregs[i][1], i);
    if (_pgp->writeRegister(&_d, FPGAregs[i][0], FPGAregs[i][1])) {
      printf("EpixSConfigurator::writeConfig failed writing FPGAregs[%u]\n", i);
      ret = Failure;
    }
    i+=1;
  }
  if (_pgp->writeRegister(&_d, EnableAutomaticRunTriggerAddr, _maintainLostRunTrigger)) {
    printf("EpixSConfigurator::writeConfig failed writing %s\n", "EnableAutomaticRunTriggerAddr");
    ret = Failure;
  }
  unsigned totalPixels = PixelsPerBank *(_s->get(EpixSConfigShadow::NumberOfReadableRowsPerAsic)+
          _config->calibrationRowCountPerASIC());
  if (_pgp->writeRegister(&_d, TotalPixelsAddr, totalPixels)) {
    printf("EpixSConfigurator::writeConfig failed writing %s\n", "TotalPixelsAddr");
    ret = Failure;
  } else {
    printf("EpixSConfigurator::writeConfig total Pixels %d\n", totalPixels);
  }
  if (_debug&1) {
    printf("EpixSConfigurator::writeConfig FPGA values:\n");
    for (unsigned i=0; i<EpixSConfigShadow::NumberOfValues; i++) {
      printf("\treg %3u writing 0x%x at 0x%x\n", i, u[i], configAddrs[i][0]);
    }
  }
  for (unsigned i=0; !ret && i<(EpixSConfigShadow::NumberOfValues); i++) {
    if (configAddrs[i][1] == ReadWrite) {
      if (_debug & 1) printf("EpixSConfigurator::writeConfig writing addr(0x%x) data(0x%x) configValue(%u)\n", configAddrs[i][0], u[i], i);
      if (_pgp->writeRegister(&_d, configAddrs[i][0], u[i])) {
        printf("EpixSConfigurator::writeConfig failed writing %s\n", _s->name((EpixSConfigShadow::Registers)i));
        ret = Failure;
      }
    } else if (configAddrs[i][1] == ReadOnly) {
      if (_debug & 1) printf("EpixSConfigurator::writeConfig reading addr(0x%x)", configAddrs[i][0]);
      if (_pgp->readRegister(&_d, configAddrs[i][0], 0x1000+i, u+i)) {
        printf("EpixSConfigurator::writeConfig failed reading %s\n", _s->name((EpixSConfigShadow::Registers)i));
        ret |= Failure;
      }
      if (_debug & 1) printf(" data(0x%x) configValue[%u]\n", u[i], i);
    }
    if (_pgp->writeRegister(&_d, DaqTrigggerDelayAddr, RunToDaqTriggerDelay+_s->get(EpixSConfigShadow::RunTrigDelay))) {
      printf("EpixSConfigurator::writeConfig failed writing DaqTrigggerDelay\n");
      ret = Failure;
    }
    microSpin(100);
  }
  /* if (ret == Success) return checkWrittenConfig(true);
  else */ return ret;
}

unsigned EpixSConfigurator::checkWrittenConfig(bool writeBack) {
  unsigned ret = Success;
  unsigned size = EpixSConfigShadow::NumberOfValues;
  uint32_t myBuffer[size];
  if (_s && _pgp) {
    _d.dest(EpixSDestination::Registers);
    for (unsigned i=0;_pgp && (i<size); i++) {
      if ((configAddrs[i][1] == ReadWrite) && _pgp) {
        if (_pgp->readRegister(&_d, configAddrs[i][0], (0x1100+i), myBuffer+i)) {
          printf("EpixSConfigurator::checkWrittenConfig failed reading %s\n", _s->name((EpixSConfigShadow::Registers)i));
          ret |= Failure;
        }
      }
    }
  }
  if (_s && _pgp) {
    if (ret == Success) {
      EpixSConfigShadow* readConfig = (EpixSConfigShadow*) myBuffer;
      uint32_t r, c;
      for (unsigned i=0; i<EpixSConfigShadow::NumberOfRegisters; i++) {
        if (EpixSConfigShadow::readOnly((EpixSConfigShadow::Registers)i) == EpixSConfigShadow::ReadWrite) {
          if ((r=readConfig->get((EpixSConfigShadow::Registers)i)) !=
              (c=_s->get((EpixSConfigShadow::Registers)i))) {
            printf("EpixSConfigurator::checkWrittenConfig failed, rw(0x%x), config 0x%x!=0x%x readback at %s. %sriting back to config.\n",
                configAddrs[i][1], c, r, _s->name((EpixSConfigShadow::Registers)i), writeBack ? "W" : "Not w");
            if (writeBack) _s->set((EpixSConfigShadow::Registers)i, r);
            ret |= Failure;
          }
        }
      }
    }
  }
  if (!_pgp) printf("EpixSConfigurator::checkWrittenConfig found nil pgp\n");
  return ret;
}


unsigned EpixSConfigurator::writeASIC() {
  unsigned ret = Success;
  _d.dest(EpixSDestination::Registers);
  unsigned amask = _config->asicMask();
  uint32_t* u = (uint32_t*)_config;
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (amask&(1<<index)) {
      u = (uint32_t*) &_config->asics(index);
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      for (unsigned i=0; i<EpixSASIC_ConfigShadow::NumberOfValues; i++) {
        if ((AconfigAddrs[i][1] == ReadWrite) || (AconfigAddrs[i][1] == WriteOnly)) {
          if (_debug & 1) printf("EpixSConfigurator::writeAsicConfig writing addr(0x%x) data(0x%x) configValue(%u) Asic(%u)\n",
              a+AconfigAddrs[i][0], u[i], i, index);
          if (_pgp->writeRegister( &_d, a+AconfigAddrs[i][0], u[i], false, Pgp::PgpRSBits::Waiting)) {
            printf("EpixSConfigurator::writeASIC failed on %s, ASIC %u\n",
                EpixSASIC_ConfigShadow::name((EpixSASIC_ConfigShadow::Registers)i), index);
            ret |= Failure;
          }
          _getAnAnswer();
        } else if (AconfigAddrs[i][1] == ReadOnly) {
          if (_debug & 1) printf("EpixSConfigurator::writeAsic reading addr(0x%x)", a+AconfigAddrs[i][0]);
          if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0x1100+i, u+i)) {
            printf("EpixSConfigurator::writeASIC failed reading 0x%x\n", a+AconfigAddrs[i][0]);
            ret |= Failure;
          }
          if (_debug & 1) printf(" data(0x%x) configValue[%u] Asic(%u)\n", u[i], i, index);
       }
      }
    }
  }
  writePixelBits();
  /* if (ret==Success) return checkWrittenASIC(true);
  else*/ return ret;
}

// from Kurtis
//  0x080000 : Row in global space (0-19)
//  0x080001 : Col in global space (0-19)
//  0x080002 : Pixel data
//  0x080003 : DNC
//  0x080004 : DNC
//  0x080005 : DNC

class block {
  public:
    block() : _row(0), _col(0), _pixel(0) {
      _b[0] =_b[1] =_b[2] = 0;
    };
    ~block() {};
    void row( uint32_t r ) {_row = r; }
    uint32_t row() { return _row; }
    void col( uint32_t c ) {_col = c; }
    uint32_t col() { return _col; }
    void pixel(uint32_t p) { _pixel = p; }
    uint32_t pixel() { return _pixel; }
    uint32_t* b() { return _b; }
  private:
    uint32_t  _row;
    uint32_t  _col;
    uint32_t  _pixel;
    uint32_t  _b[3];
};

// From Kurtis ...
//    1) set SaciClkBit to 4 (register 0x000028)
//    2) send ASIC command: PrepareMultiConfig (0x808000, 0x908000, 0xA08000, 0xB08000
//         for ASICs 0,1,2,3 respectively)  [data written doesn't matter]
//    3) loop over columns 0-95:
//       3a) write ASIC register: ColCounter (0x806013, 0x906013, 0xA06013, 0xB06013
//            for ASICs 0,1,2,3 respectively) [data is the column number]
//       3b) write ASIC register: WriteColData (0x803000, 0x903000, 0xA03000, 0xB03000
//            for ASICs 0,1,2,3 respectively) - [bits 1,0 are mask, test, respectively]
//    4) send ASIC command: PrepareForReadout: (0x800000, 0x900000, 0xA00000, 0xB00000)
//    5) (optional) SaciClkBit needs to be set to 3 to do individual pixel writes, so
//          you could set it back to 3 here, or incorporate that as part of the single
//          pixel write procedure
//
//Bit 16 of the row data to flag the write as targeting a calibration row.
//Bit 17 of the row data would indicate whether the row is on top (0) or bottom (1).
//So row on top you would do the same as the existing block write for 4x pixels, but with row: 0x00010000
//And row on bottom would do the same as the existing block write for 4x pixels, but with row: 0x00030000

unsigned EpixSConfigurator::writePixelBits() {
  enum { writeAhead = 68, pixelMask = 0xf, numbPixValues = 16 };
  unsigned ret = Success;
  _d.dest(EpixSDestination::Registers);
  unsigned amask    = _config->asicMask();
  unsigned rows = _config->numberOfRows();
  unsigned cols = _config->numberOfColumns();
  unsigned synchDepthHisto[writeAhead];
  unsigned pops[numbPixValues] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned writesNeeded = 0;
  block blk;
  for (unsigned i=0; i<writeAhead; i++) {
    synchDepthHisto[i] = 0;
  }
  // find the most popular pixel setting
  for (unsigned r=0; r<rows; r++) {
    for (unsigned c=0; c<cols; c++) {
      pops[_config->asicPixelConfigArray()[r][c]&pixelMask] += 1;
    }
  }
  unsigned max = 0;
  uint32_t pixel = _config->asicPixelConfigArray()[0][0] & 0xffff;
  printf("\nEpixSConfigurator::writePixelBits() histo");
  for (unsigned n=0; n<numbPixValues; n++) {
    printf(" %u,", pops[n]);
    writesNeeded += pops[n];
    if (pops[n] > max) {
      max = pops[n];
      pixel = n;
    }
  }
  writesNeeded -= max;
  printf(" pixel %u, writes %u\n", pixel, writesNeeded);
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 4, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("EpixSConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  // program the entire array to the most popular setting
  // note, that this is necessary because the global pixel write does not work in this device and programming
  //   one pixel in one bank also programs the same pixel in the other banks
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (amask&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareMultiConfigAddr, 0, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
        printf("EpixSConfigurator::writePixelBits failed on ASIC %u, writing prepareMultiConfig\n", index);
        ret |= Failure;
      }
    }
  }
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (amask&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+WriteEntireMatrixAddr, pixel, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
        printf("EpixSConfigurator::writePixelBits failed on ASIC %u, writing entire matrix %u\n", index, pixel);
        ret |= Failure;
      }
    }
  }
  // now program the ones that were not the same
  //  NB must program four at a time because that's how the asic works
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 3, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("EpixSConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  unsigned writeCount = 0;
  // allow it queue up writeAhead commands
  Pds::Pgp::ConfigSynch mySynch(_fd, writeAhead, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t) + 5);
  for (unsigned row=0; row<rows; row++) {
    blk.row(row);
    for (unsigned col=0; col<cols; col++) {
      uint32_t thisPix;
      if ((thisPix = (_config->asicPixelConfigArray()[row][col]&pixelMask)) != pixel) {
        blk.col(col);
        blk.pixel(thisPix);
        if (mySynch.take() == false) {
          printf("EpixSConfigurator::writePixelBits synchronization failed on col %u write %u\n", col%(cols>>1), writeCount);
          ret |= Failure;
        } else {
          if (_pgp->writeRegisterBlock(
              &_d,
              MultiplePixelWriteCommandAddr,
              (unsigned*)&blk,
              sizeof(block)/sizeof(uint32_t),
              writesNeeded>writeAhead ? Pds::Pgp::PgpRSBits::Waiting : Pds::Pgp::PgpRSBits::notWaiting,
              (_debug&1)?true:false))
          {
            printf("EpixSConfigurator::writePixelBits failed on row %u, col %u, pixel %u\n", row, col, thisPix);
            ret |= Failure;
          }
        }
//        printf("EpixSConfigurator::writePixelBits row %u, col %u, pixel %u\n", row, col, thisPix);
        synchDepthHisto[mySynch.depth()] += 1;
        writeCount += 1;
      }
    }
  }
  printf("EpixSConfigurator::writePixelBits write count %u\n", writeCount);
//  if (mySynch.clear() == false) {
//    printf("EpixSConfigurator::writePixelBits synchronization failed to clear\n");
//  }
  // NB we are dividing the calibration rows by two because
  //  there is one pixel configuration row per each pair of calibration rows.
  for (unsigned row=0; row<(_config->numberOfCalibrationRows()>>1); row++) {
    blk.row(row ? BottomCalibRow : TopCalibRow);
    printf("EpixSConfigurator::writePixelBits calibration row 0x%x\n", blk.row());
    for (unsigned col=0; col<cols; col++) {
      blk.col(col);
      blk.pixel(_config->calibPixelConfigArray()[row][col] & 3);
//        if (mySynch.take() == false) {
//          printf("EpixSConfigurator::writePixelBits synchronization failed on write of calib row %u\n", row);
//          ret |= Failure;
//        }
        if (_pgp->writeRegisterBlock(
            &_d,
            MultiplePixelWriteCommandAddr,
            (unsigned*)&blk,
            sizeof(block)/sizeof(uint32_t),
            Pds::Pgp::PgpRSBits::notWaiting,
            (_debug&1)?true:false))
        {
          printf("EpixSConfigurator::writePixelBits failed on calib row %u, col %u pixel %u\n", row, col, blk.pixel());
          ret |= Failure;
        }
    }
  }
//  if (mySynch.clear() == false) {
//    printf("EpixSConfigurator::writePixelBits synchronization failed to clear\n");
//  }
  printf(" SynchBufferDepth:\n");
  for (unsigned i=0; i<writeAhead; i++) {
    if (synchDepthHisto[i] > 1) {
      printf("\t%3u - %5u\n", i, synchDepthHisto[i]);
    }
  }
  printf("EpixSConfigurator::writePixelBits PrepareForReadout for ASIC:");
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (amask&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareForReadout, 0, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
        printf("EpixSConfigurator::writePixelBits failed on ASIC %u\n", index);
        ret |= Failure;
      }
      printf(" %u", index);
    }
  }
  printf("\n");
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, SaciClkBitValue, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("EpixSConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  return ret;
}


bool EpixSConfigurator::_getAnAnswer(unsigned size, unsigned count) {
  Pds::Pgp::RegisterSlaveImportFrame* rsif;
  count = 0;
  while ((count < 6) && (rsif = _pgp->read(size)) != 0) {
    if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting) {
//      printf("_getAnAnswer in %u tries\n", count+1);
      return true;
    }
    count += 1;
  }
  return false;
}

unsigned EpixSConfigurator::checkWrittenASIC(bool writeBack) {
  unsigned ret = Success;
  bool done = false;
  while (!done && _config && _pgp) {
    _d.dest(EpixSDestination::Registers);
    unsigned amask = _config->asicMask();
    uint32_t myBuffer[sizeof(EpixSASIC_ConfigShadow)/sizeof(uint32_t)];
    EpixSASIC_ConfigShadow* readAsic = (EpixSASIC_ConfigShadow*) myBuffer;
    for (unsigned index=0; index<_config->numberOfAsics(); index++) {
      if (amask&(1<<index)) {
        uint32_t a = AsicAddrBase + (AsicAddrOffset * index);
        for (int i = 0; (i<EpixSASIC_ConfigShadow::NumberOfValues) && (ret==Success); i++) {
          if ((AconfigAddrs[i][1] != WriteOnly) && _pgp) {
            if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0xa000+(index<<2)+i, myBuffer+i, 1)){
              printf("EpixSConfigurator::checkWrittenASIC read reg %u failed on ASIC %u at 0x%x\n",
                  i, index, a+AconfigAddrs[i][0]);
              return Failure;
            }
            myBuffer[i] &= 0xffff;
          }
        }
        EpixSASIC_ConfigShadow* confAsic = (EpixSASIC_ConfigShadow*) &( _config->asics(index));
        if ( (ret==Success) && (*confAsic != *readAsic) && _pgp) {
          printf("EpixSConfigurator::checkWrittenASIC failed on ASIC %u\n", index);
          ret = Failure;
          if (writeBack) *confAsic = *readAsic;
        }
      }
    }
    done = true;
  }
  if (!_pgp) printf("EpixSConfigurator::checkWrittenASIC found nil pgp\n");
  return ret;
}

void EpixSConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    uint32_t count = sequenceCount();
    uint32_t acount = acquisitionCount();
    uint32_t lId   = lowerCarrierId();
    uint32_t uId   = upperCarrierId();
    printf("\tSequence Count(%u), Acquisition Count(%u) lower,upper IDs (0x%x,0x%x)\n", count, acount, lId, uId);

  }
  if (_debug & 0x200) {
    printf("Checking Configuration, no news is good news ...\n");
//    if (Failure == checkWrittenConfig(false)) {
//      printf("EpixSConfigurator::checkWrittenConfig() FAILED !!!\n");
//    }
//    enableRunTrigger(false);
//    if (Failure == checkWrittenASIC(false)) {
//      printf("EpixSConfigurator::checkWrittenASICConfig() FAILED !!!\n");
//    }
//    enableRunTrigger(true);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x300) {
    printf("EpixSConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
