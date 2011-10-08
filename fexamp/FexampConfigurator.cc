/*
 * FexampConfigurator.cc
 *
 *  Created on: September 23, 2011
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
#include "pds/fexamp/FexampConfigurator.hh"
#include "pds/fexamp/FexampDestination.hh"
#include "pds/fexamp/FexampStatusRegisters.hh"
#include "pds/config/FexampConfigType.hh"

using namespace Pds::Fexamp;

class FexampDestination;

static uint32_t configAddrs[FexampConfigType::NumberOfValues] = {
    0x00,
    0x010,
    0x011,
    0x012,
    0x013,
    0x014,
    0x015
};

FexampConfigurator::FexampConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _testModeState(0), _runControl(0), _rhisto(0) {
  _statRegs.pgp = pgp();
  printf("FexampConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

FexampConfigurator::~FexampConfigurator() {}

void FexampConfigurator::resetFrontEnd(uint32_t r) {
  _d.dest(FexampDestination::VC2);
  _pgp->writeRegister(&_d, ResetAddr, r);
  if (!(r&MasterReset)) {
    microSpin(10);
    _pgp->writeRegister(&_d, ResetAddr, 0);
  }
}

void FexampConfigurator::resetSequenceCount() {
  _d.dest(FexampDestination::VC1);
  _pgp->writeRegister(&_d, RunControlAddr, _runControl | SequenceCountResetMask);
  microSpin(10);
  _pgp->writeRegister(&_d, RunControlAddr, _runControl);
}

uint32_t FexampConfigurator::sequenceCount() {
  _d.dest(FexampDestination::VC1);
  uint32_t count=1111;
  _pgp->readRegister(&_d, SequenceCountAddr, 0x5e4, &count);
  return (count);
}

void FexampConfigurator::enableExternalTrigger(bool f) {
  _d.dest(FexampDestination::VC1);
  _runControl = f ? _runControl | ExternalTriggerEnableMask : _runControl & (~ExternalTriggerEnableMask);
  _pgp->writeRegister(&_d, RunControlAddr, _runControl);
}

void FexampConfigurator::print() {}

void FexampConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool FexampConfigurator::_flush(unsigned index=0) {
  enum {numberOfTries=5};
  unsigned version = 0;
  unsigned failCount = 0;
  bool ret = false;
  printf("\n\t--flush-%u-", index);
  while ((failCount++<numberOfTries) && (Failure == _statRegs.readVersion(&version))) {
    printf("%s(%u)-", _d.name(), failCount);
  }
  if (failCount<numberOfTries) {
    printf("%s version(0x%x)\n\t", _d.name(), version);
  }
  else {
    ret = true;
    printf("\n\t");
  }
  return ret;
}

unsigned FexampConfigurator::configure( FexampConfigType* c, unsigned mask) {
  _config = c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  bool printFlag = !(mask & 0x2000);
  if (printFlag) printf("Fexamp Config");
  printf(" config(%p) mask(0x%x)\n", _config, ~mask);
  unsigned ret = 0;
  mask = ~mask;
  clock_gettime(CLOCK_REALTIME, &start);
  resetFrontEnd(Fexamp::MasterReset);
  _flush();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  if (printFlag) printf("\n\twriting top level config");
  ret |= writeConfig();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  ret <<= 1;
  if (printFlag) printf("\n\twriting ASIC regs");
  ret |= writeASIC();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  ret <<= 1;
  uint32_t version;
  if (Failure == _statRegs.readVersion(&version)) {
    printf("FexampConfigurator::configure failed to read FPGA version\n");
    ret |= 1;
  } else {
    _config->FPGAVersion(version);
  }
  if (usleep(10000)<0) perror("FexampConfigurator::configure second ulseep failed\n");
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone \n", ret);
    printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
    /*if (ret)*/ dumpFrontEnd();
  }
  return ret;
}

unsigned FexampConfigurator::writeConfig() {
  _d.dest(FexampDestination::VC1);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  for (unsigned i=0; !ret && i<FexampConfigType::NumberOfValues; i++) {
    if (_pgp->writeRegister(&_d, configAddrs[i], u[i])) {
      printf("FexampConfigurator::writeConfig failed writing %u\n", i);
      ret = Failure;
    }
  }
  uint32_t v;
  ret |= _statRegs.readVersion(&v);
  _config->FPGAVersion(v);
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned FexampConfigurator::checkWrittenConfig(bool writeBack) {
  _d.dest(FexampDestination::VC1);
  unsigned ret = Success;
  unsigned size = FexampConfigType::NumberOfValues;
  uint32_t myBuffer[size];
  for (unsigned i=0; i<size; i++) {
    if (_pgp->readRegister(&_d, configAddrs[i], 0x1100+i, myBuffer+i)) {
      printf("FexampConfigurator::checkWrittenConfig failed reading %u\n", i);
      ret |= Failure;
    }
  }
  if (ret == Success) {
    FexampConfigType* readConfig = (FexampConfigType*) myBuffer;
    uint32_t r, c;
    for (unsigned i=0; i<FexampConfigType::NumberOfRegisters; i++) {
      if ((r=readConfig->get((FexampConfigType::Registers)i)) !=
          (c=_config->get((FexampConfigType::Registers)i))) {
        printf("FexampConfigurator::checkWrittenConfig failed, 0x%x!=0x%x at offset %u. %sriting back to config.\n",
            c, r, i, writeBack ? "W" : "Not w");
        if (writeBack) _config->set((FexampConfigType::Registers)i, r);
        ret |= Failure;
      }
    }
  }
  return ret;
}


unsigned FexampConfigurator::writeASIC() {
  unsigned ret = Success;
  _d.dest(FexampDestination::VC1);
  for (unsigned index=0; index<FexampConfigType::NumberOfASICs; index++) {
    if (_pgp->writeRegisterBlock(
        &_d,
        ASICbaseAddr,
        (uint32_t*) &(_config->ASICs()[index]),
        ASIC_V1::NumberOfValues)) {
      printf("FexampConfigurator::writeASIC block failed on ASIC %u\n", index);
      ret |= Failure;
    }
    uint32_t* up = (uint32_t*)_config->ASICs()[index].channels();
    for (unsigned i=0; i<ASIC_V1::NumberOfChannels && ret==Success; i++) {
      if (_pgp->writeRegister(&_d, (ChannelBaseAddr+ASIC_V1::NumberOfChannels-1)-i, up[i])) {
        printf("FexampConfigurator::writeASIC failed on Channel %u\n", i);
        ret |= Failure;
      }
    }
  }
  _pgp->writeRegister(&_d, AsicShiftReqAddr, 1);
  if (ret==Success) return checkWrittenASIC(true);
  else return ret;
}

unsigned FexampConfigurator::checkWrittenASIC(bool writeBack) {
  _d.dest(FexampDestination::VC1);
  uint32_t myBuffer[sizeof(ASIC_V1)/sizeof(uint32_t)];
  ASIC_V1* ra = (ASIC_V1*)myBuffer;
  unsigned ret = Success;
  for (unsigned index=0; index<FexampConfigType::NumberOfASICs; index++) {
    uint32_t* up = (uint32_t*)ra->channels();
    for (int i=ASIC_V1::NumberOfChannels-1; i>=0 && ret==Success; i--) {
      if (_pgp->readRegister(&_d, ChannelBaseAddr+(ASIC_V1::NumberOfChannels-1-i), 0xc00+i, &up[i])) {
        printf("FexampConfigurator::checkWrittenASIC failed reading Channel %d\n", i);
        ret = Failure;
      }
    }
    if (_pgp->readRegister(&_d, ASICbaseAddr, 0xa51c+index, myBuffer, ASIC_V1::NumberOfValues)){
      printf("FexampConfigurator::checkWrittenASIC read block failed on ASIC %u\n", index);
      return Failure;
    }
    if ( (ret==Success) && (_config->ASICs()[index] != *ra) ) {
      ret = Failure;
      printf("FexampConfigurator::checkWrittenASIC failed, %sriting back to config.\n", writeBack ? "W" : "Not w");
      if (writeBack) _config->ASICs()[index] = *ra;
    }
  }
  return ret;
}

void FexampConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    ret = _statRegs.read();
    if (ret == Success) {
      _statRegs.print();
      uint32_t count = FexampConfigurator::sequenceCount();
      printf("\tSequenceCount(%u)\n", count);
    } else {
      printf("\tFexamp Status Registers could not be read!\n");
    }
  }
  if (_debug & 0x400) {
    printf("Checking Configuration, no news is good news ...\n");
    if (Failure == checkWrittenConfig(false)) {
      printf("FexampConfigurator::checkWrittenConfig() FAILED !!!\n");
    } if (Failure == checkWrittenASIC(false)) {
      printf("FexampConfigurator::checkWrittenASICConfig() FAILED !!!\n");
    }
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("FexampConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
