/*
 * EpixConfigurator.cc
 *
 *  Created on: 2013.11.9
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
#include "pds/pgp/Configurator.hh"
#include "pds/epix/EpixConfigurator.hh"
#include "pds/epix/EpixDestination.hh"
#include "pds/epix/EpixStatusRegisters.hh"
#include "pds/config/EpixConfigType.hh"

using namespace Pds::Epix;

class EpixDestination;

//  configAddrs array elements are address:useModes

static uint32_t configAddrs[EpixConfigShadow::NumberOfValues][2] = {
    {0x00,  1}, //  version
    {0x02,  0}, //  RunTrigDelay
    {0x04,  2}, //  DaqTrigDelay
    {0x07,  0}, //  DacSetting
    {0x29,  0}, //  AsicPins
    {0x2a,  0}, //  AsicPinControl
    {0x20,  0}, //  AcqToAsicR0Delay
    {0x21,  0}, //  AsicR0ToAsicAcq
    {0x22,  0}, //  AsicAcqWidth
    {0x23,  0}, //  AsicAcqLToPPmatL
    {0x24,  0}, //  AsicRoClkHalfT
    {0x25,  0}, //  AdcReadsPerPixel
    {0x26,  0}, //  AdcClkHalfT
    {0x2b,  0}, //  AsicR0Width
    {0x2c,  0}, //  AdcPipelineDelay
    {0x2e,  0}, //  PrepulseR0Width
    {0x2f,  0}, //  PrepulseR0Delay
    {0x30,  1}, //  DigitalCardId0
    {0x31,  1}, //  DigitalCardId1
    {0x32,  1}, //  AnalogCardId0
    {0x33,  1}, //  AnalogCardId1
    {0,     2}, //  LastRowExclusions
    {0,     2}, //  NumberOfAsicsPerRow
    {0,     2}, //  NumberOfAsicsPerColumn
    {0,     2}, //  NumberOfRowsPerAsic
    {0,     2}, //  NumberOfPixelsPerAsicRow
    {0x10,  1}, //  BaseClockFrequency
    {0xd,   0},  //  AsicMask
};

//0x01 0x801001 0x00000007  From Kurtis
//0x01 0x801002 0x00000003
//0x01 0x801003 0x000013ff
//0x01 0x801004 0x00000010
//0x01 0x801005 0x00000003
//0x01 0x801006 0x00000000
//0x01 0x801007 0x0000001e
//0x01 0x801008 0x00000073
//0x01 0x801009 0x00000044
//0x01 0x80100a 0x000000b7
//0x01 0x80100b 0x00000047
//0x01 0x80100c 0x00000001
//0x01 0x80100d 0x0000006c
//0x01 0x80100e 0x00000071
//0x01 0x80100f 0x00000071
//0x01 0x801010 0x00000000
//0x01 0x801012 0x00000061
//0x01 0x801014 0x0000005f
//0x01 0x806013 0x0000005f
//0x01 0x808000 0x00000000
//0x01 0x808000 0x00000000
//0x01 0x804000 0x40000000
//0x01 0x804000 0x40000000


static uint32_t AconfigAddrs[EpixASIC_ConfigShadow::NumberOfValues][2] = {
    {0x1001,  0},  //  monostPulser
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
    {0x1015,  1}   //  chipID
};


EpixConfigurator::EpixConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _testModeState(0), _config(0), _s(0), _rhisto(0) {
  printf("EpixConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

EpixConfigurator::~EpixConfigurator() {}

void EpixConfigurator::resetFrontEnd() {
  _d.dest(EpixDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, ResetAddr, 1);
    microSpin(10);
  } else printf("EpixConfigurator::resetFrontEnd() found nil _pgp so not reset\n");
}

void EpixConfigurator::resetSequenceCount() {
  _d.dest(EpixDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, ResetFrameCounter, 1);
    _pgp->writeRegister(&_d, ResetAcqCounter, 1);
    microSpin(10);
  } else printf("EpixConfigurator::resetSequenceCount() found nil _pgp so not reset\n");
}

uint32_t EpixConfigurator::sequenceCount() {
  uint32_t count=1111;
  _d.dest(EpixDestination::Registers);
  if (_pgp) {
    _pgp->readRegister(&_d, ReadFrameCounter, 0x5e4, &count);
  } else printf("EpixConfigurator::sequenceCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t EpixConfigurator::acquisitionCount() {
  uint32_t count=1111;
  _d.dest(EpixDestination::Registers);
  if (_pgp) {
    _pgp->readRegister(&_d, ReadAcqCounter, 0x5e4, &count);
  } else printf("EpixConfigurator::acquisitionCount() found nil _pgp so not read\n");
  return (count);
}

void EpixConfigurator::enableExternalTrigger(bool f) {
  _d.dest(EpixDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, DaqTriggerEnable, f ? Enable : Disable);
  } else printf("EpixConfigurator::enableExternalTrigger(%s) found nil _pgp so not set\n", f ? "true" : "false");
}

void EpixConfigurator::enableRunTrigger(bool f) {
  _d.dest(EpixDestination::Registers);
  _pgp->writeRegister(&_d, RunTriggerEnable, f ? Enable : Disable);
}

void EpixConfigurator::print() {}

void EpixConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

void EpixConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("EpixConfigurator::runTimeConfigName(%s)\n", name);
}

bool EpixConfigurator::_flush(unsigned index) {
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

unsigned EpixConfigurator::configure( EpixConfigType* c, unsigned first) {
  _config = c;
  _s = (EpixConfigShadow*) c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  bool printFlag = true;
  if (printFlag) printf("Epix Config size(%u)", c->_sizeof());
  printf(" config(%p) first(%u)\n", _config, first);
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  printf("EpixConfigurator::configure %sreseting front end\n", first ? "" : "not ");
  if (first) {
   // resetFrontEnd();
  }
  if (_flush()) {
    printf("EpixConfigurator::configure determined that we lost contact with the front end, exiting!\n");
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
  if (usleep(10000)<0) perror("EpixConfigurator::configure second ulseep failed\n");
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
    {EnableAutomaticRunTriggerAddr, 1, 0},  // remove when in config
    {EnableAutomaticDaqTriggerAddr, 0, 0},
    {0,0,1}
};

unsigned EpixConfigurator::writeConfig() {
  _d.dest(EpixDestination::Registers);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  unsigned i=0;
  printf("\n");
  while ((ret == Success) && (FPGAregs[i][2] == 0)) {
    if (_debug & 1) printf("EpixConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", FPGAregs[i][0], FPGAregs[i][1], i);
    if (_pgp->writeRegister(&_d, FPGAregs[i][0], FPGAregs[i][1])) {
      printf("EpixConfigurator::writeConfig failed writing FPGAregs[%u]\n", i);
      ret = Failure;
    }
    i+=1;
  }
  if (_pgp->writeRegister(&_d, TotalPixelsAddr, PixelsPerBank * (_s->get(EpixConfigShadow::NumberOfRowsPerAsic)-1))) {
    printf("EpixConfigurator::writeConfig failed writing %s\n", "TotalPixelsAddr");
    ret = Failure;
  }
  if (_debug&1) {
    printf("EpixConfigurator::writeConfig FPGA values:\n");
    for (unsigned i=0; i<EpixConfigShadow::NumberOfValues; i++) {
      printf("\t%3u 0x%x\n", i, u[i]);
    }
  }
  for (unsigned i=0; !ret && i<EpixConfigShadow::NumberOfValues; i++) {
    if (configAddrs[i][1] == ReadWrite) {
      if (_debug & 1) printf("EpixConfigurator::writeConfig writing addr(0x%x) data(0x%x) configValue(%u)\n", configAddrs[i][0], u[i], i);
      if (_pgp->writeRegister(&_d, configAddrs[i][0], u[i])) {
        printf("EpixConfigurator::writeConfig failed writing %s\n", _s->name((EpixConfigShadow::Registers)i));
        ret = Failure;
      }
    } else if (configAddrs[i][1] == ReadOnly) {
      if (_debug & 1) printf("EpixConfigurator::writeConfig reading addr(0x%x)", configAddrs[i][0]);
      if (_pgp->readRegister(&_d, configAddrs[i][0], 0x1000+i, u+i)) {
        printf("EpixConfigurator::writeConfig failed reading %s\n", _s->name((EpixConfigShadow::Registers)i));
        ret |= Failure;
      }
      if (_debug & 1) printf(" data(0x%x) configValue[%u]\n", u[i], i);
    }
    if (_pgp->writeRegister(&_d, DaqTrigggerDelayAddr, RunToDaqTriggerDelay+_s->get(EpixConfigShadow::RunTrigDelay))) {
      printf("EpixConfigurator::writeConfig failed writing DaqTrigggerDelay\n");
      ret = Failure;
    }
    microSpin(100);
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned EpixConfigurator::checkWrittenConfig(bool writeBack) {
  unsigned ret = Success;
  unsigned size = EpixConfigShadow::NumberOfValues;
  uint32_t myBuffer[size];
  bool done = false;
  while (!done && _config && _pgp) {
    _d.dest(EpixDestination::Registers);
    for (unsigned i=0; i<size; i++) {
      if (configAddrs[i][1] == ReadWrite) {
        if (_pgp->readRegister(&_d, configAddrs[i][0], 0x1100+i, myBuffer+i)) {
          printf("EpixConfigurator::checkWrittenConfig failed reading %s\n", _s->name((EpixConfigShadow::Registers)i));
          ret |= Failure;
        }
      }
    }
    done = true;
  }
  if (ret == Success  && _pgp) {
    EpixConfigShadow* readConfig = (EpixConfigShadow*) myBuffer;
    uint32_t r, c;
    for (unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++) {
      if (EpixConfigShadow::readOnly((EpixConfigShadow::Registers)i) == EpixConfigShadow::ReadWrite) {
        if ((r=readConfig->get((EpixConfigShadow::Registers)i)) !=
            (c=_s->get((EpixConfigShadow::Registers)i))) {
          printf("EpixConfigurator::checkWrittenConfig failed, rw(%u), 0x%x!=0x%x at %s. %sriting back to config.\n",
              configAddrs[i][1], c, r, _s->name((EpixConfigShadow::Registers)i), writeBack ? "W" : "Not w");
          if (writeBack) _s->set((EpixConfigShadow::Registers)i, r);
          ret |= Failure;
        }
      }
    }
  }
  if (!_pgp) printf("EpixConfigurator::checkWrittenConfig found nil pgp\n");
  return ret;
}

static uint32_t maskLineRegs[5][3] = {
    {0x0,    0x0, 0},
    {0x8000, 0x0, 0},
    {0x6011, 0x0, 0},
    {0x2000, 0x2, 0},
    {0x0,    0x0, 1}
};

unsigned EpixConfigurator::writeASIC() {
  unsigned ret = Success;
  _d.dest(EpixDestination::Registers);
  unsigned m = _config->asicMask();
  uint32_t* u = (uint32_t*)_config;
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      u = (uint32_t*) &_config->asics(index);
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      for (unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfValues; i++) {
        if ((AconfigAddrs[i][1] == ReadWrite) || (AconfigAddrs[i][1] == WriteOnly)) {
          if (_debug & 1) printf("EpixConfigurator::wAsicConfig writing addr(0x%x) data(0x%x) configValue(%u) Asic(%u)\n",
              a+AconfigAddrs[i][0], u[i], i, index);
          if (_pgp->writeRegister( &_d, a+AconfigAddrs[i][0], u[i])) {
            printf("EpixConfigurator::writeASIC failed on %s, ASIC %u\n",
                EpixASIC_ConfigShadow::name((EpixASIC_ConfigShadow::Registers)i), index);
            ret |= Failure;
          }
        } else if (AconfigAddrs[i][1] == ReadOnly) {
          if (_debug & 1) printf("EpixConfigurator::wAsicConfig reading addr(0x%x)", a+AconfigAddrs[i][0]);
          if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0x1100+i, u+i)) {
            printf("EpixConfigurator::checkWrittenConfig failed reading %s\n",
                EpixASIC_ConfigShadow::name((EpixASIC_ConfigShadow::Registers)i));
            ret |= Failure;
          }
          if (_debug & 1) printf(" data(0x%x) configValue[%u] Asic(%u)\n", u[i], i, index);
       }
      }
      unsigned test = _config->asicPixelTestArray()[index][0][0];
      unsigned mask = _config->asicPixelMaskArray()[index][0][0];
      uint32_t bits = (test ? 1 : 0) | (mask ? 2 : 0);
      for (unsigned i=0; i<RepeatControlCount; i++) {
        if (_pgp->writeRegister( &_d, a+WritePixelCommand, 0)) {
          printf("EpixConfigurator::writeASIC WritePixelCommand failed on ASIC %u\n", index);
          ret |= Failure;
        }
      }
      for (unsigned i=0; i<RepeatControlCount; i++) {
        if (_pgp->writeRegister( &_d, a+GloalPixelCommand, bits)) {
          printf("EpixConfigurator::writeASIC failed on ASIC %u\n", index);
          ret |= Failure;
        }
      }
      unsigned j=0;
      while ((ret == Success) && (maskLineRegs[j][2] == 0)) {
        if (_debug & 1) printf("EpixConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", a+maskLineRegs[j][0], maskLineRegs[j][1], j);
        for (unsigned i=0; (ret == Success) && (i<RepeatControlCount); i++) {
          if (_pgp->writeRegister(&_d, a+maskLineRegs[j][0], maskLineRegs[j][1])) {
            printf("EpixConfigurator::writeConfig failed writing maskLineRegs[%u]\n", j);
            ret = Failure;
          }
        }
        j+=1;
      }
    }
  }
  /* if (ret==Success) return checkWrittenASIC(true);
  else*/ return ret;
}

unsigned EpixConfigurator::checkWrittenASIC(bool writeBack) {
  unsigned ret = Success;
  bool done = false;
  while (!done && _config && _pgp) {
    _d.dest(EpixDestination::Registers);
    unsigned m = _config->asicMask();
    uint32_t myBuffer[sizeof(EpixASIC_ConfigShadow)/sizeof(uint32_t)];
    EpixASIC_ConfigShadow* readAsic = (EpixASIC_ConfigShadow*) myBuffer;
    for (unsigned index=0; index<_config->numberOfAsics(); index++) {
      if (m&(1<<index)) {
        uint32_t a = AsicAddrBase + (AsicAddrOffset * index);
        for (int i = 0; (i<EpixASIC_ConfigShadow::NumberOfValues) && (ret==Success); i++) {
          if ((AconfigAddrs[i][1] != WriteOnly) && _pgp) {
            if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0xa000+(index<<2)+i, myBuffer+i, 1)){
              printf("EpixConfigurator::checkWrittenASIC read reg %u failed on ASIC %u at 0x%x\n",
                  i, index, a+AconfigAddrs[i][0]);
              return Failure;
            }
            myBuffer[i] &= 0xffff;
          }
        }
        EpixASIC_ConfigShadow* confAsic = (EpixASIC_ConfigShadow*) &( _config->asics(index));
        if ( (ret==Success) && (*confAsic != *readAsic) ) {
          printf("EpixConfigurator::checkWrittenASIC failed on ASIC %u\n", index);
          ret = Failure;
          if (writeBack) *confAsic = *readAsic;
        }
      }
    }
    done = true;
  }
  if (!_pgp) printf("EpixConfigurator::checkWrittenASIC found nil pgp\n");
  return ret;
}

void EpixConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    uint32_t count = EpixConfigurator::sequenceCount();
    uint32_t acount = EpixConfigurator::acquisitionCount();
    printf("\tSequenceCount(%u), Acquisition Count(%u)\n", count, acount);
  }
  if (_debug & 0x400) {
    printf("Checking Configuration, no news is good news ...\n");
    if (Failure == checkWrittenConfig(false)) {
      printf("EpixConfigurator::checkWrittenConfig() FAILED !!!\n");
    }
//    enableRunTrigger(false);
//    if (Failure == checkWrittenASIC(false)) {
//      printf("EpixConfigurator::checkWrittenASICConfig() FAILED !!!\n");
//    }
//    enableRunTrigger(true);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("EpixConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
