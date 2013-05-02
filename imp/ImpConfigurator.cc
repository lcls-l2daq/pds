/*
 * ImpConfigurator.cc
 *
 *  Created on: April 12, 2013
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
#include "pds/imp/ImpConfigurator.hh"
#include "pds/imp/ImpDestination.hh"
#include "pds/imp/ImpStatusRegisters.hh"
#include "pds/config/ImpConfigType.hh"

using namespace Pds::Imp;

class ImpDestination;

static uint32_t configAddrs[ImpConfigType::NumberOfValues] = {
      0x6, //    Range,
      0x7, //    Cal_range,
      0x8, //    Reset,
      0x9, //    Bias_data,
      0xA, //    Cal_data,
      0xB, //    BiasDac_data,
      0xE, //    Cal_strobe,
      0x1B,//    NumberOfSamples,
      0x10,//    TrigDelay,
      0x16,//    Adc_delay,
};

ImpConfigurator::ImpConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _testModeState(0), _runControl(0), _rhisto(0) {
  _statRegs.pgp = pgp();
  printf("ImpConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

ImpConfigurator::~ImpConfigurator() {}

//void ImpConfigurator::resetFrontEnd(uint32_t r) {
//  _d.dest(ImpDestination::VC2);
//  _pgp->writeRegister(&_d, ResetAddr, r);
//  if (!(r&MasterReset)) {
//    microSpin(10);
//    _pgp->writeRegister(&_d, ResetAddr, 0);
//  }
//}

void ImpConfigurator::resetSequenceCount() {
  _d.dest(ImpDestination::CommandVC);
  // belt and suspenders, delete the second if and when the first works
  _pgp->writeRegister(&_d, resetAddr, CountRestMask);
  _pgp->writeRegister(&_d, sequenceCountAddr, 0);
}

uint32_t ImpConfigurator::sequenceCount() {
  _d.dest(ImpDestination::CommandVC);
  uint32_t count=1111;
  _pgp->readRegister(&_d, sequenceCountAddr, 0x5e4, &count);
  return (count);
}

void ImpConfigurator::print() {}

void ImpConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool ImpConfigurator::_flush(unsigned index=0) {
//  enum {numberOfTries=5};
//  unsigned version = 0;
//  unsigned failCount = 0;
  bool ret = false;
//  printf("\n\t--flush-%u-", index);
//  while ((failCount++<numberOfTries) && (Failure == _statRegs.readVersion(&version))) {
//    printf("%s(%u)-", _d.name(), failCount);
//  }
//  if (failCount<numberOfTries) {
//    printf("%s version(0x%x)\n\t", _d.name(), version);
//  }
//  else {
    ret = true;
//    printf("\n\t");
//  }
  return ret;
}

unsigned ImpConfigurator::configure( ImpConfigType* c, unsigned mask) {
  _config = c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  bool printFlag = !(mask & 0x2000);
  if (printFlag) printf("Imp Config");
  printf(" config(%p) mask(0x%x)\n", _config, ~mask);
  unsigned ret = 0;
  mask = ~mask;
  clock_gettime(CLOCK_REALTIME, &start);
//  resetFrontEnd(Imp::MasterReset);
//  _flush();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  if (printFlag) printf("\n\twriting config");
  ret |= writeConfig();
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  }
  ret <<= 1;
//  resetSequenceCount();
//  if (usleep(10000)<0) perror("ImpConfigurator::configure second ulseep failed\n");
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone, %s \n", ret, ret ? "FAILED" : "SUCCEEDED");
    printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
    /*if (ret)*/ dumpFrontEnd();
  }
  return ret;
}

unsigned ImpConfigurator::writeConfig() {
  _d.dest(ImpDestination::CommandVC);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  for (unsigned i=0; !ret && i<ImpConfigType::NumberOfValues; i++) {
    if (_pgp->writeRegister(&_d, configAddrs[i], u[i])) {
      printf("ImpConfigurator::writeConfig failed writing %u\n", i);
      ret = Failure;
    }
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned ImpConfigurator::checkWrittenConfig(bool writeBack) {
  _d.dest(ImpDestination::CommandVC);
  unsigned ret = Success;
  unsigned size = ImpConfigType::NumberOfValues;
  uint32_t myBuffer[size];
  for (unsigned i=0; i<size; i++) {
    if (_pgp->readRegister(&_d, configAddrs[i], 0x1100+i, myBuffer+i)) {
      printf("ImpConfigurator::checkWrittenConfig failed reading %u\n", i);
      ret |= Failure;
    }
  }
  if (ret == Success) {
    ImpConfigType* readConfig = (ImpConfigType*) myBuffer;
    uint32_t r, c;
    for (unsigned i=0; i<size; i++) {
      if ((r=readConfig->get((ImpConfigType::Registers)i)) !=
          (c=_config->get((ImpConfigType::Registers)i))) {
        printf("ImpConfigurator::checkWrittenConfig failed, 0x%x!=0x%x at offset %u. %sriting back to config.\n",
            c, r, i, writeBack ? "W" : "Not w");
        if (writeBack) _config->set((ImpConfigType::Registers)i, r);
        ret |= Failure;
      }
    }
  }
  return ret;
}


void ImpConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    ret = _statRegs.read();
    if (ret == Success) {
      _statRegs.print();
      uint32_t count = ImpConfigurator::sequenceCount();
      printf("\tSequenceCount(%u)\n", count);
    } else {
      printf("\tImp Status Registers could not be read!\n");
    }
  }
  if (_debug & 0x400 && _pgp) {
    printf("Checking Configuration, no news is good news ...\n");
    if (Failure == checkWrittenConfig(false)) {
      printf("ImpConfigurator::checkWrittenConfig() FAILED !!!\n");
    }
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("ImpConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
