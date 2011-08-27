/*
 * XampsConfigurator.cc
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

//        The order isn't critical, but I will add a section in the document with
//        a detailed flowchart showing which registers to write along with the
//        values to write.   Basically, we write the switcher control register
//        (value = 0xB), then write the asic control registers, then set the h.v.
//        The Readout Timing registers come up in a working state, so unless the
//        default values change in the future they don't need to be written.
//              from Joe Meade
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
//#include "pgpcard/PgpCardMod.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/pgp/Configurator.hh"
#include "pds/xamps/XampsConfigurator.hh"
#include "pds/xamps/XampsDestination.hh"
#include "pds/xamps/XampsInternalRegisters.hh"
#include "pds/xamps/XampsExternalRegisters.hh"
#include "pds/config/XampsConfigType.hh"

namespace Pds {
  namespace Xamps {

    class XampsDestination;
    class Pds::Pgp::AddressRange;
    class ASIC;

     static uint32_t externalAddrs[XampsConfigType::NumberOfRegisters] = {
        // NB if these are to be included they should be uncommented ConfigV1.cc/hh and XampsConfigurator.cc
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
//        0x07,
        0x0a,
        0x10,
        0x40,
        0x70,
        0x71,
        0x80,
        0x81,
        0x82,
        0x83,
        0x84,
        0x85,
        0x86
    };

    XampsConfigurator::XampsConfigurator( XampsConfigType* c, int f, unsigned d) :
                   Pds::Pgp::Configurator(f, d),
                   _testModeState(0), _config(c), _rhisto(0) {
       printf("XampsConfigurator constructor _config(%p)\n", _config);
      //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
      //    _rhisto = (unsigned*) calloc(1000, 4);
      //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
    }

    XampsConfigurator::~XampsConfigurator() {}

    void XampsConfigurator::print() {}

    void XampsConfigurator::printMe() {
      printf("Configurator: ");
      for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
      printf("\n");
    }

    bool XampsConfigurator::_flush(unsigned index=0) {
      enum {numberOfTries=5};
      unsigned version = 0;
      unsigned failCount = 0;
      bool ret = false;
      printf("\n\t--flush-%u-", index);
      _d.dest(XampsDestination::Internal);
      while ((Failure == _pgp->readRegister(
          &_d,
          RegisterAddr,
          0x55000,
          &version)) && failCount++<numberOfTries ) {
        printf("%s(%u)-", _d.name(), failCount);
      }
      if (failCount<numberOfTries) {
        printf("%s version(0x%x)\n\t", _d.name(), version);
      }
      else {
        ret = true;
        printf("\n\t");
      }
      failCount = 0;
      XampsExternalRegisters eregs = XampsExternalRegisters::XampsExternalRegisters(_pgp);
      _d.dest(XampsDestination::External);
      while ((Failure == eregs.readVersion((uint32_t*)&version)) && failCount++<numberOfTries ) {
        printf("%s[%u]-", _d.name(), failCount);
      }
      if (failCount<numberOfTries) {
        printf("%s version[0x%x]\n\t", _d.name(), version);
      }
      else {
        ret = true;
        printf("\n\t");
      }
      return ret;
    }

    unsigned XampsConfigurator::configure(unsigned mask) {
      timespec      start, end, sleepTime, shortSleepTime;
      sleepTime.tv_sec = 0;
      sleepTime.tv_nsec = 25000000; // 25ms
      shortSleepTime.tv_sec = 0;
      shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
      bool printFlag = !(mask & 0x2000);
      if (printFlag) printf("Xamps Config");
      printf(" config(%p) mask(0x%x)\n\tResetting front end", _config, ~mask);
      unsigned ret = 0;
      mask = ~mask;
      clock_gettime(CLOCK_REALTIME, &start);
      _d.dest(XampsDestination::Internal);
      ret |= _pgp->writeRegister(&_d, resetAddr, MasterResetMask);
      nanosleep(&sleepTime, 0);
      _flush();
      ret <<= 1;
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("\t- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
      }
      if (printFlag) printf("\n\twriting switcher control");
      ret |= _pgp->writeRegister(&_d, SwitcherControlAddr, SwitcherControlValue);
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
      }
      ret <<= 1;
      if (printFlag) printf("\n\twriting ASIC regs");
      ret |= writeASICs();
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
      }
      ret <<= 1;
      if (printFlag) printf("\n\twriting external regs");
      ret |= writeConfig();
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
      }
      ret <<= 1;
      _d.dest(XampsDestination::External);
      if (_pgp->writeRegister(&_d, ClearFrameCountAddr, ClearFrameCountValue)) {
        printf("XampsConfigurator::configure writing clear frame count failed!\n");
        ret |= 1;
      }
      ret <<= 1;
      if (_pgp->writeRegister(&_d, ClearFrameCountAddr, 0)) {
        printf("XampsConfigurator::configure clearing clear frame count failed!\n");
        ret |= 1;
      }
      ret <<= 1;
      _d.dest(XampsDestination::Internal);
      uint32_t version;
      if (_pgp->readRegister(&_d, RegisterAddr, 0x55000, &version)) {
        printf("XampsConfigurator::configure failed to read FPGA version\n");
        ret |= 1;
      } else {
        _config->FPGAVersion(version);
      }
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - \n\tdone \n", ret);
        printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
        if (ret) dumpFrontEnd();
      }
      return ret;
    }

    unsigned XampsConfigurator::writeConfig() {
      _d.dest(XampsDestination::External);
      uint32_t* u = (uint32_t*)_config;
      _testModeState = *u;
      unsigned ret = Success;
      for (unsigned i=0; i<XampsConfigType::NumberOfRegisters; i++) {
        if (_pgp->writeRegister(&_d, externalAddrs[i], u[i] & XampsConfigType::rangeHigh((XampsConfigType::Registers)i))) {
          printf("XampsConfigurator::writeConfig failed writing %u\n", i);
          ret = Failure;
        }
        if ((externalAddrs[i] < ReadoutTimingUpdateAddr) && (externalAddrs[i+1] > ReadoutTimingUpdateAddr)) {
          if (_pgp->writeRegister(&_d, ReadoutTimingUpdateAddr, 1)) {
            printf("XampsConfigurator::writeConfig failed writing on ReadoutTimingUpdateAddr\n");
            ret = Failure;
          }
        }
      }
      if (ret == Success) return checkWrittenConfig(true);
      else return ret;
    }

    unsigned XampsConfigurator::checkWrittenConfig(bool writeBack) {
      _d.dest(XampsDestination::External);
      uint32_t* u = (uint32_t*)_config;
      unsigned ret = Success;
      unsigned size = XampsConfigType::NumberOfRegisters;
      uint32_t myBuffer[size];
//      _pgp->readRegister(&_d, 0, 0x6969, myBuffer, 7); printf("\n");
//      _pgp->readRegister(&_d, 10, 0x6969, myBuffer+10); printf("\n");
//      for (unsigned i=0; i<7; i++) printf("0x%x (%u)\n", myBuffer[i], myBuffer[i]); printf("0x%x (%u)\n", myBuffer[10], myBuffer[10]);
      for (unsigned i=0; i<size; i++) {
        if (_pgp->readRegister(&_d, externalAddrs[i], 0x101, myBuffer+i)) {
          printf("XampsConfigurator::checkWrittenConfig failed reading %u\n", i);
          ret |= Failure;
        } else if ((myBuffer[i] & (uint32_t)XampsConfigType::rangeHigh((XampsConfigType::Registers)i)) != u[i]) {
          printf("XampsConfigurator::checkWrittenConfig failed, 0x%x!=0x%x at offset %u. %sriting back to config.\n",
              u[i],
              myBuffer[i],
              i,
              writeBack ? "W" : "Not w");
          ret |= Failure;
//          if (writeBack) u[i] = myBuffer[i] & XampsConfigType::rangeHigh((XampsConfigType::Registers)i);
        }
      }
      return ret;
    }

    unsigned XampsConfigurator::writeASICs() {
      _d.dest(XampsDestination::External);
      for (unsigned index=0; index<XampsConfigType::NumberOfASICs; index++) {
        if (_pgp->writeRegisterBlock(
            &_d,
            ASIC_topBaseAddr - index * ASIC_addrOffset,
            (uint32_t*) &(_config->ASICs()[index]),
            sizeof(XampsASIC)/sizeof(uint32_t))) {
          printf("XampsConfigurator::writeASICs failed on ASIC %u\n", index);
          return Failure;
        }
      }
      if (_pgp->writeRegister(
          &_d,
          ASIC_triggerAddr,
          ASIC_triggerValue)) {
        printf("XampsConfigurator::writeASICs failed on writing the trigger!\n");
        return Failure;
      }
      return Success; //checkWrittenASICs(true);
    }

    unsigned XampsConfigurator::checkWrittenASICs(bool writeBack) {
      _d.dest(XampsDestination::External);
      uint32_t* u;
      unsigned size = sizeof(XampsASIC)/sizeof(uint32_t);
      uint32_t myBuffer[size];
      unsigned ret = Success;
      for (unsigned index=0; index<XampsConfigType::NumberOfASICs; index++) {
        u = (uint32_t*)  &(_config->ASICs()[index]);
        if (_pgp->readRegister(&_d, ASIC_topBaseAddr - index * ASIC_addrOffset, 0xa000+index, myBuffer, size)){
          printf("XampsConfigurator::checkWrittenASICs read back failed on ASIC %u\n", index);
          return Failure;
        }
        for (unsigned i=0; i<size; i++) {
          if (myBuffer[i] != u[i]) {
            ret = Failure;
            printf("XampsConfigurator::checkWrittenASIC failed, 0x%x!=0x%x at offset %u. %sriting back to config.\n",
                u[i],
                myBuffer[i],
                i,
                writeBack ? "W" : "Not w");
            if (writeBack) u[i] = myBuffer[i];
          }
        }
      }
      return ret;
    }

    void XampsConfigurator::dumpFrontEnd() {
      timespec      start, end;
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned ret = Success;
      if (_debug & 0x100) {
        _d.dest(XampsDestination::Internal);
        XampsInternalRegisters regs;
        ret = _pgp->readRegister(
            &_d,
            RegisterAddr,
            0x666,
            (uint32_t*)&regs,
            sizeof(XampsInternalRegisters)/sizeof(unsigned));
        if (ret == Success) {
          regs.print();
        } else {
          printf("\tXamps Internal Registers could not be read!\n");
        }
        XampsExternalRegisters eregs = XampsExternalRegisters::XampsExternalRegisters(_pgp);
        if (eregs.read() == Success) {
          eregs.print();
        }
      }
      if (_debug & 0x400) {
        printf("Checking Configuration, no news is good news ...\n");
        if (Failure == checkWrittenConfig(false)) {
          printf("XampsConfigurator::checkWrittenConfig() FAILED !!!\n");
        }
        if (Failure == checkWrittenASICs(false)) {
          printf("XampsConfigurator::checkWrittenASICs() FAILED !!!\n");
        }
      }
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      if (_debug & 0x700) {
        printf("XampsConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
        printf(" - %s\n", ret == Success ? "Success" : "Failed!");
      }
      return;
    }
  } // namespace Xamps
} // namespace Pds

