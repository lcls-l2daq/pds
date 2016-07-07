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

static uint32_t configAddrs[Pds::ImpConfig::NumberOfValues] = {
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
  printf("ImpConfigurator constructor\n");
  strcpy(_runTimeConfigFileName, "");
  new(&_statRegs) ImpStatusRegisters();
  _statRegs.pgp = pgp();

  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

ImpConfigurator::~ImpConfigurator() {}

void ImpConfigurator::resetFrontEnd(uint32_t r) {
  _d.dest(ImpDestination::CommandVC);
  _pgp->writeRegister(&_d, resetAddr, r);
  usleep(125000);
}

void ImpConfigurator::resetSequenceCount() {
  _d.dest(ImpDestination::CommandVC);
  // belt and suspenders, delete the second if and when the first works
  if (_pgp) {
    _pgp->writeRegister(&_d, resetAddr, CountResetMask);
    _pgp->writeRegister(&_d, sequenceCountAddr, 0);
  } else printf("ImpConfigurator::sequenceCount() found nil _pgp so not reset\n");
}

uint32_t ImpConfigurator::sequenceCount() {
  _d.dest(ImpDestination::CommandVC);
  uint32_t count=1111;
  if (_pgp) {
    _pgp->readRegister(&_d, sequenceCountAddr, 0x5e4, &count);
  } else printf("ImpConfigurator::sequenceCount() found nil _pgp so not read\n");
  return (count);
}

void ImpConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("ImpConfigurator::runTimeConfigName(%s)\n", name);
}

void ImpConfigurator::print() {}

void ImpConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool ImpConfigurator::_flush(unsigned index=0) {
  return false;
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
  resetFrontEnd(Imp::SoftReset);
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
  loadRunTimeConfigAdditions(_runTimeConfigFileName);
  ret <<= 1;
//  resetSequenceCount();
//  if (usleep(10000)<0) perror("ImpConfigurator::configure second ulseep failed\n");
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone, %s \n", ret, ret ? "FAILED" : "SUCCEEDED");
    printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
//    /*if (ret)*/ dumpFrontEnd();
  }
  return ret;
}

unsigned ImpConfigurator::writeConfig() {
  _d.dest(ImpDestination::CommandVC);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  for (unsigned i=0; !ret && i<Pds::ImpConfig::NumberOfValues; i++) {
    if (_pgp->writeRegister(&_d, configAddrs[i], u[i])) {
      printf("ImpConfigurator::writeConfig failed writing 0x%x to %u address in step %u\n", u[i], configAddrs[i], i);
      ret = Failure;
    }
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned ImpConfigurator::checkWrittenConfig(bool writeBack) {
  _d.dest(ImpDestination::CommandVC);
  unsigned ret = Success;
  unsigned size = Pds::ImpConfig::NumberOfValues;
  uint32_t myBuffer[size];
  for (unsigned i=0; (i<size) && _pgp && _config; i++) {
    if (_pgp->readRegister(&_d, configAddrs[i], 0x1100+i, myBuffer+i)) {
      printf("ImpConfigurator::checkWrittenConfig failed reading %u\n", i);
      ret |= Failure;
    }
  }
  if (ret == Success  && _config && _pgp) {
    ImpConfigType* readConfig = (ImpConfigType*) myBuffer;
    uint32_t r, c;
    for (unsigned i=0; i<size; i++) {
      if ((r=Pds::ImpConfig::get(*readConfig,(ImpConfigType::Registers)i)) !=
          (c=Pds::ImpConfig::get(*_config   ,(ImpConfigType::Registers)i))) {
        printf("ImpConfigurator::checkWrittenConfig failed, 0x%x!=0x%x at offset %u. %sriting back to config.\n",
            c, r, i, writeBack ? "W" : "Not w");
        if (writeBack) Pds::ImpConfig::set(*_config,(ImpConfigType::Registers)i, r);
        ret |= Failure;
      }
    }
  }
  if (!_pgp) printf("ImpConfigurator::checkWrittenConfig found nil pgp\n");
  if (!_config) printf("ImpConfigurator::checkWrittenConfig found nil config\n");
  return ret;
}


void ImpConfigurator::dumpFrontEnd() {
  int ret = Success;
    ret = _statRegs.read();
    if (ret == Success) {
      _statRegs.print();
      uint32_t count = 1111;
      count = ImpConfigurator::sequenceCount();
      printf("\tSequenceCount(%u)\n", count);
    } else {
      printf("\tImp Status Registers could not be read!\n");
    }
  dumpPgpCard();
  return;
}
