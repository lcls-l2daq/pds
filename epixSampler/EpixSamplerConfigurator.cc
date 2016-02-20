/*
 * EpixSamplerConfigurator.cc
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
#include "pds/epixSampler/EpixSamplerConfigurator.hh"
#include "pds/epixSampler/EpixSamplerDestination.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerConfigV1.hh"

using namespace Pds::EpixSampler;

class EpixSamplerDestination;

static unsigned configAddrs[EpixSamplerConfigShadow::NumberOfValues] = {
    0x00,    //  version
    0x02,    //  RunTrigDelay
    0x04,    //  DaqTrigDelay
    0x07,    //  DacSetting
    0x26,    //  AdcClkHalfT
    0x2c,    //  AdcPipelineDelay
    0x30,    //  DigitalCardId0
    0x31,    //  DigitalCardId1
    0x32,    //  AnalogCardId0
    0x33,    //  AnalogCardId1
    0,       //  NumberOfChannels
    0,       //  SamplesPerChannel
    0x10,    //  BaseClockFrequency
    0x2a     //  TestPatternEnable
};

EpixSamplerConfigurator::EpixSamplerConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _config(0), _s(0), _rhisto(0) {
  printf("EpixSamplerConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

EpixSamplerConfigurator::~EpixSamplerConfigurator() {}

void EpixSamplerConfigurator::resetFrontEnd(uint32_t r) {
  _d.dest(EpixSamplerDestination::Registers);
  _pgp->writeRegister(&_d, ResetAddr, r);
  microSpin(10);
}


void EpixSamplerConfigurator::resetSequenceCount() {
  _d.dest(EpixSamplerDestination::Registers);
  unsigned count = 0x1234;
  _pgp->readRegister(&_d, ReadFrameCounter, 0xedfa, &count);
  printf("Epix Sampler state:\n\tSequenceCount      (%u)\n", count);
  _pgp->readRegister(&_d, DaqTriggerEnable, 0xedfb, &count);
  printf("\tDaqTriggerEnable   (%u)\n", count);
  _pgp->readRegister(&_d, ReadAcqCounter, 0xedfc, &count);
  printf("\tAcquisitionCount   (%u)\n", count);
  _pgp->readRegister(&_d, RunTriggerEnable, 0xedfd, &count);
  printf("\tRunTriggerEnable   (%u)\n", count);
  _pgp->writeRegister(&_d, ResetFrameCounter, 0);
}

uint32_t EpixSamplerConfigurator::sequenceCount() {
  _d.dest(EpixSamplerDestination::Registers);
  uint32_t count=1111;
  _pgp->readRegister(&_d, ReadFrameCounter, 0x5e4, &count);
  return (count);
}

uint32_t EpixSamplerConfigurator::acquisitionCount() {
  _d.dest(EpixSamplerDestination::Registers);
  uint32_t count=2222;
  _pgp->readRegister(&_d, ReadAcqCounter, 0x5e5, &count);
  return (count);
}

unsigned EpixSamplerConfigurator::firmwareVersion(unsigned* v) {
  _d.dest(EpixSamplerDestination::Registers);
  EpixSamplerConfigShadow::Registers r = EpixSamplerConfigShadow::Version;
  *v = 3333;
  return _pgp->readRegister(&_d, configAddrs[r], 0x5e6, v);
}

void EpixSamplerConfigurator::enableExternalTrigger(bool f) {
  _d.dest(EpixSamplerDestination::Registers);
  _pgp->writeRegister(&_d, DaqTriggerEnable, f ? Enable : Disable);
}

void EpixSamplerConfigurator::print() {}

void EpixSamplerConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool EpixSamplerConfigurator::_flush(unsigned index=0) {
  enum {numberOfTries=5};
  unsigned version = 0;
  unsigned failCount = 0;
  bool ret = false;
  printf("\n\t--flush-%u-", index);
  while ((failCount++<numberOfTries) && (Failure == firmwareVersion(&version))) {
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

unsigned EpixSamplerConfigurator::configure( EpixSamplerConfigType* c, unsigned mask) {
  _config = c; _s = (EpixSamplerConfigShadow*) c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  printf("EpixSampler Config config(%p)\n", c);
  unsigned ret = 0;
  mask = ~mask;
  unsigned count = 0x1234;
  _pgp->readRegister(&_d, ReadFrameCounter, 0xedfa, &count);
  printf("Epix Sampler state:\n\tSequenceCount      (%u)\n", count);
  _pgp->readRegister(&_d, DaqTriggerEnable, 0xedfb, &count);
  printf("\tDaqTriggerEnable   (%u)\n", count);
  _pgp->readRegister(&_d, ReadAcqCounter, 0xedfc, &count);
  printf("\tAcquisitionCount   (%u)\n", count);
  _pgp->readRegister(&_d, RunTriggerEnable, 0xedfd, &count);
  printf("\tRunTriggerEnable   (%u)\n", count);
  clock_gettime(CLOCK_REALTIME, &start);
//  resetFrontEnd();
//  _flush();
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
  printf("\n\twriting top level config");
  ret |= writeConfig();
  clock_gettime(CLOCK_REALTIME, &end);
  diff = timeDiff(&end, &start) + 50000LL;
//  printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
//  if (usleep(10000)<0) perror("EpixSamplerConfigurator::configure second ulseep failed\n");
//  clock_gettime(CLOCK_REALTIME, &end);
//  diff = timeDiff(&end, &start) + 50000LL;
//  printf("- 0x%x - \n\tdone \n", ret);
  printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
  /*if (ret)*/ dumpFrontEnd();
  return ret;
}

unsigned EpixSamplerConfigurator::writeConfig() {
  _d.dest(EpixSamplerDestination::Registers);
//  EpixSamplerConfigShadow::Registers r = (EpixSamplerConfigShadow::Registers)0;
  unsigned value = 0;
  unsigned ret = Success;
  unsigned* u = (uint32_t*)_config;
  unsigned r = 0;
  for (int i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++ ) {
    printf("%x ", u[i]);
  }
  printf("\n");
  if (_pgp->writeRegister(&_d, PowerEnableAddr, PowerEnableValue)) {
    printf("EpixSamplerConfigurator::writeConfig kludge failed PowerEnable\n");
    ret = Failure;
  }
  if (_pgp->writeRegister(&_d, RunTriggerEnable, Enable)) {
    printf("EpixSamplerConfigurator::writeConfig failed writing RunTriggerEnable\n");
    ret = Failure;
  }
  microSpin(1000);
  for (r=0; !ret && r<EpixSamplerConfigShadow::NumberOfValues; r++) {
    switch (_s->readOnly((EpixSamplerConfigShadow::Registers)r)) {
      case (EpixSamplerConfigShadow::ReadWrite) :
          printf("EpixSamplerConfigurator::writeConfig write register 0x%x with 0x%x\n", configAddrs[r], u[r]);
        if (_pgp->writeRegister(&_d, configAddrs[r], u[r])) {
          printf("EpixSamplerConfigurator::writeConfig failed writing %s\n", _s->name((EpixSamplerConfigShadow::Registers)r));
          ret = Failure;
        }
        break;
      case (EpixSamplerConfigShadow::ReadOnly) :
       if (_pgp->readRegister(&_d, configAddrs[r], 0xbee, &value)) {
          printf("EpixSamplerConfigurator::writeConfig failed reading %s\n", _s->name((EpixSamplerConfigShadow::Registers)r));
          ret = Failure;
        } else {
          u[r] = value;
        }
//        printf("EpixSamplerConfigurator: ReadOnly %u %s(0x%x)\n", r,  _s->name((EpixSamplerConfigShadow::Registers)r), u[r]);
        break;
      case (EpixSamplerConfigShadow::UseOnly) :
        printf("EpixSamplerConfigurator::writeConfig UseOnly %s(0x%x)\n", _s->name((EpixSamplerConfigShadow::Registers)r), u[r]);
       break;
      default:
        printf("EpixSamplerConfigurator::writeConfig failed, unknown readOnly mode %u\n", r);
        break;
    }
    microSpin(1000);
  }
  // turn on adcStreamMode bit
  if (_pgp->writeRegister(&_d, configAddrs[r-1], u[r-1] | 0x80)) {
    printf("EpixSamplerConfigurator::writeConfig kludge failed writing 1 to bit 7 of pin control\n");
    ret = Failure;
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned EpixSamplerConfigurator::checkWrittenConfig(bool writeBack) {
  _d.dest(EpixSamplerDestination::Registers);
  unsigned ret = Success;
  unsigned size = EpixSamplerConfigShadow::NumberOfValues;
  uint32_t myBuffer[size];
  for (unsigned i=0; i<size; i++) {
    if (_s->readOnly((EpixSamplerConfigShadow::Registers)i) == EpixSamplerConfigShadow::ReadWrite) {
      if (_pgp == 0) { return Success; }
      if (_pgp->readRegister(&_d, configAddrs[i], 0x1100+i, myBuffer+i)) {
        printf("EpixSamplerConfigurator::checkWrittenConfig failed reading %u\n", i);
        ret |= Failure;
      }
    }
  }
  if (ret == Success) {
    EpixSamplerConfigShadow* readConfig = (EpixSamplerConfigShadow*) myBuffer;
    uint32_t r, c;
    for (unsigned i=0; i<EpixSamplerConfigShadow::NumberOfRegisters-1; i++) {
      if (_s->readOnly((EpixSamplerConfigShadow::Registers)i) == EpixSamplerConfigShadow::ReadWrite) {
        EpixSamplerConfigShadow::Registers reg = (EpixSamplerConfigShadow::Registers)i;
        if ( _s->readOnly(reg) == EpixSamplerConfigShadow::ReadWrite) {
          if ((r=readConfig->get(reg)) != (c=_s->get(reg))) {
            printf("EpixSamplerConfigurator::checkWrittenConfig failed, 0x%x!=0x%x on %s. %sriting back to config.\n",
                c, r,  _s->name(reg), writeBack ? "W" : "Not w");
            if (writeBack) _s->set(reg, r);
            ret |= Failure;
          }
        }
      }
    }
  }
  return ret;
}


void EpixSamplerConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  _d.dest(EpixSamplerDestination::Registers);
  unsigned* u = (uint32_t*)_config;
  if (_debug & 0x100) {
    uint32_t count = 0;
//    for (int i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++ ) {
//      printf("%x ", u[i]);
//    } printf("\n");
    if (_pgp) {
      _pgp->readRegister(&_d, ReadFrameCounter, 0xedfa, &count);
      printf("Epix Sampler state:\n\tSequenceCount      (%u)\n", count);
    }
    if (_pgp) {
      _pgp->readRegister(&_d, DaqTriggerEnable, 0xedfb, &count);
      printf("\tDaqTriggerEnable   (%u)\n", count);
    }
    if (_pgp) {
      _pgp->readRegister(&_d, ReadAcqCounter, 0xedfc, &count);
      printf("\tAcquisitionCount   (%u)\n", count);
    }
    if (_pgp) {
      _pgp->readRegister(&_d, RunTriggerEnable, 0xedfd, &count);
      printf("\tRunTriggerEnable   (%u)\n", count);
    }
   if (_s) {
      for (int i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++ ) {
        EpixSamplerConfigShadow::Registers r = (EpixSamplerConfigShadow::Registers)i;
        printf("\t%s(0x%x)\n", _s->name(r), _s->get(r));
      }
   }
   if (u) {
     for (int i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++ ) {
       printf("%x ", u[i]);
     }
     printf("\n");
   }
   //    printf("\tnumberOfChannels   (0x%x)\n", _config->numberOfChannels());
   //    printf("\tsamplesPerChanne   (0x%x)\n", _config->samplesPerChannel());
  }
  if (_debug & 0x400) {
    printf("Checking Configuration, no news is good news ...\n");
    if (Failure == checkWrittenConfig(false)) {
      printf("EpixSamplerConfigurator::checkWrittenConfig() FAILED !!!\n");
    }
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("EpixSamplerConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
