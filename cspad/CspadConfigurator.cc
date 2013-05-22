/*
 * CspadConfigurator.cc
 *
 *  Created on: Nov 15, 2010
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
#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadDestination.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/config/CsPadConfigType.hh"

#include "TestData.cc"

namespace Pds {
  namespace CsPad {

    class CspadQuadRegisters;
    class CspadConcentratorRegisters;
    class CspadDestination;
    class ProtectionSystemThreshold;
    class Pds::Pgp::AddressRange;

    unsigned                  CspadConfigurator::_quadAddrs[] = {
        0x110001,  // shiftSelect0    1
        0x110007,  // shiftSelect1    2
        0x110008,  // shiftSelect2    3
        0x110009,  // shiftSelect3    4
        0x110002,  // edgeSelect0     5
        0x11000a,  // edgeSelect1     6
        0x11000b,  // edgeSelect2     7
        0x11000c,  // edgeSelect3     8
        0x110003,  // readClkSet      9
        0x110005,  // readClkHold    10
        0x110006,  // dataMode       11
        0x300000,  // prstSel        12
        0x300001,  // acqDelay       13
        0x300002,  // intTime        14
        0x300003,  // digDelay       15
        0x300005,  // ampIdle        16
        0x300006,  // injTotal       17
        0x400001,  // rowColShiftPer 18
        0x300009,  // ampReset       19
        0x30000b,  // digCount       20
        0x30000a   // digPeriod      21  //sizeOfQuadWrite in Cspadconfigurator.hh
    };

    unsigned                   CspadConfigurator::_quadReadOnlyAddrs[] = {
        0x400000,  // shiftTest       1
        0x500000   // version         2 //sizeOfQuadReadOnly in Cspadconfigurator.hh
    };

    CspadConfigurator::CspadConfigurator( CsPadConfigType* c, int f, unsigned d) :
                   Pds::Pgp::Configurator(f, d),
                   _config(c), _rhisto(0),
                   _conRegs(new CspadConcentratorRegisters()),
                   _quadRegs(new CspadQuadRegisters()) {
      CspadConcentratorRegisters::configurator(this);
      CspadQuadRegisters::configurator(this);
      _initRanges();
      strcpy(_runTimeConfigFileName, "");
       printf("CspadConfigurator constructor _config(%p), quadMask(0x%x)\n",
         _config, (unsigned)_config->quadMask());
      //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
      //    _rhisto = (unsigned*) calloc(1000, 4);
      //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
    }

    CspadConfigurator::~CspadConfigurator() {
      delete (_conRegs);
      delete (_quadRegs);
    }

    void CspadConfigurator::print() {}

    void CspadConfigurator::printMe() {
      printf("Configurator: ");
      for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
      printf("\n");
    }

    void CspadConfigurator::runTimeConfigName(char* name) {
      if (name) strcpy(_runTimeConfigFileName, name);
      printf("CspadConfigurator::runTimeConfigName(%s)\n", name);
    }

    void CspadConfigurator::_initRanges() {
      new ((void*)&_gainMap) Pds::Pgp::AddressRange(0x000000, 0x010000);
      new ((void*)&_digPot)  Pds::Pgp::AddressRange(0x200000, 0x210000);
    }

    bool CspadConfigurator::_flush(unsigned index) {
      enum {numberOfTries=5};
      unsigned version = 0;
      unsigned failCount = 0;
      bool ret = false;
      printf("--flush-%u-", index);
      _d.dest(CspadDestination::CR);
      while ((Failure == _pgp->readRegister(
          &_d,
          ConcentratorVersionAddr,
          0x55000,
          &version)) && failCount++<numberOfTries ) {
        printf("%s(%u)-", _d.name(), failCount);
      }
      if (failCount<numberOfTries) printf("%s(0x%x)\n", _d.name(), version);
      else ret = true;
      for (unsigned q=0; q<Pds::CsPad::MaxQuadsPerSensor; q++) {
        if ((1<<q) & _config->quadMask()) {
          unsigned version = 0;
          failCount = 0;
          _d.dest(q);
          while ((Failure == _pgp->readRegister(
              &_d,
              0x500000,  // version address
              0x50000 + (q<<12),
              &version)) && failCount++<numberOfTries ) {
            printf("[%s:%u]-", _d.name(), failCount);
          }
          if (failCount<numberOfTries) printf("%s[0x%x]\n", _d.name(), version);
          else ret = true;
        }
      }
      printf("\n");
      return ret;
    }

    unsigned CspadConfigurator::configure(CsPadConfigType* config, unsigned mask) {
      _config = config;
      timespec      start, end, sleepTime, shortSleepTime;
      sleepTime.tv_sec = 0;
      sleepTime.tv_nsec = 25000000; // 25ms
      shortSleepTime.tv_sec = 0;
      shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
      bool printFlag = !(mask & 0x2000);
      if (printFlag) printf("Cspad Config");
      printf(" config(%p) quadMask(0x%x) mask(0x%x)\n", &_config, (unsigned)_config->quadMask(), ~mask);
      unsigned ret = 0;
      mask = ~mask;
      clock_gettime(CLOCK_REALTIME, &start);
      // N.B. We MUST stop running before resetting anything, preventing garbaging the FE pipeline
      _d.dest(CspadDestination::CR);
      ret |= _pgp->writeRegister(&_d, RunModeAddr, Pds::CsPad::NoRunning);
      nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
      //        ret |= _pgp->writeRegister(&_d, resetQuadsAddr, 1);
//      clock_gettime(CLOCK_REALTIME, &end); printf("\n\ttxClock %d %ld\n", (int)end.tv_sec, end.tv_nsec);
      _pgp->writeRegister(&_d, resetAddr, 1);
      nanosleep(&sleepTime, 0); //nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
      if (_flush(0)) printf("CspadConfigurator::configure _flush(0) FAILED\n");
      ret |= _pgp->writeRegister(&_d, TriggerWidthAddr, TriggerWidthValue);
      ret |= _pgp->writeRegister(&_d, ResetSeqCountRegisterAddr, 1);
      ret |= _pgp->writeRegister(&_d, externalTriggerDelayAddr, externalTriggerDelayValue);
      ret <<= 1;
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
      }
      if (mask&1) {
        if (printFlag) printf("\nConfiguring:\n\twriting control regs - 0x%x - \n\twriting regs", ret);
        ret |= writeRegs();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
      ret <<= 1;
      if (mask&2 && ret==0) {
        if (printFlag) printf(" - 0x%x - \n\twriting pots ", ret);
        ret |= writeDigPots();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
      ret <<= 1;
//      if (mask&4 && ret==0) {
//        if (printFlag) printf("- 0x%x - \n\twriting test data[%u] ", ret, (unsigned)_config->tdi());
//        ret |= writeTestData();
//        if (printFlag) {
//          clock_gettime(CLOCK_REALTIME, &end);
//          uint64_t diff = timeDiff(&end, &start) + 50000LL;
//          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
//        }
//      }
      ret <<= 1;
      if (mask&8 && ret==0) {
        if (printFlag) printf("- 0x%x - \n\twriting gain map ", ret);
        ret |= writeGainMap();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
      ret <<= 1;
      loadRunTimeConfigAdditions(_runTimeConfigFileName);
      if (mask&16 && ret==0) {
        if (printFlag) printf("- 0x%x - \n\treading ", ret);
//        ret |= readRegs();                    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        readRegs();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
      ret <<= 1;
      _d.dest(CspadDestination::CR);
      ret |= _pgp->writeRegister(&_d, EnableEvrAddr, EnableEvrValue);
      ret |= _pgp->writeRegister(&_d, resetCountersAddr, 1);
      ret |= _pgp->writeRegister(&_d, RunModeAddr, _config->inactiveRunMode());
      //read  version to flush out the replies to the above writes.
      unsigned version;
      _pgp->readRegister(
                &_d,
                ConcentratorVersionAddr,
                0x55000,
                &version);
      usleep(25000);
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - \n\tdone \n", ret);
        printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
        //      printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
        //    print();
        if (ret) dumpFrontEnd();
      }
      return ret;
    }

    void CspadConfigurator::dumpFrontEnd() {
      timespec      start, end;
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned ret = Success;
      if (_debug & 0x100) {
        printf("CspadConfigurator::dumpFrontEnd: Concentrator\n");
        ret = _conRegs->read();
        if (ret == Success) {
          _conRegs->print();
        } else {
          printf("\tCould not be read!\n");
        }
      }
      if (_debug & 0x200) {
        for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor && ret == Success; i++) {
          unsigned ret2 = 0;
          if ((1<<i) & _config->quadMask()) {
            printf("CspadConfigurator::dumpFrontEnd: Quad %u\n", i);
            ret2 = _quadRegs->read(i);
            if (ret2 == Success) {
              _quadRegs->print();
            } else {
              printf("\tCould not be read!\n");
            }
            ret |= ret2;
          }
        }
      }
      if (_debug & 0x400) {
        printf("Checking Configuration, no news is good news ...\n");
        if (Failure == checkWrittenRegs()) {
          printf("CspadConfigurator::checkWrittenRegs() FAILED !!!\n");
        }
        if (Failure == checkDigPots()) {
          printf("CspadConfigurator::checkDigPots() FAILED !!!!\n");
        }
      }
      dumpPgpCard();
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      if (_debug & 0x700) {
        printf("CspadConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
        printf(" - %s\n", ret == Success ? "Success" : "Failed!");
      }
      return;
    }

    unsigned CspadConfigurator::readRegs() {
      unsigned ret = Success;
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          uint32_t* u = (uint32_t*) _config->quads()[i].readOnly();
          _d.dest(i);
          for (unsigned j=0; j<sizeOfQuadReadOnly; j++) {
            if (Failure == _pgp->readRegister(&_d, _quadReadOnlyAddrs[j], 0x55000 | (i<<4) | j, u+j)) {
              ret = Failure;
            }
          }
          if (_debug & 0x10) printf("CspadConfigurator Quad %u read only 0x%x 0x%x %p %p\n",
              i, (unsigned)u[0], (unsigned)u[1], u, _config->quads()[i].readOnly());
        }
      }
      _d.dest(CspadDestination::CR);
      if (Failure == _pgp->readRegister(
          &_d,
          ConcentratorVersionAddr,
          0x55000,
          _config->concentratorVersionAddr())) {
        ret = Failure;
      }
      if (_debug & 0x10) printf("\nCspadConfigurator concentrator version 0x%x\n", *_config->concentratorVersionAddr());
      return ret;
    }

    unsigned CspadConfigurator::writeRegs() {
      uint32_t* u;
      _d.dest(CspadDestination::CR);
      if (_pgp->writeRegister(&_d, EventCodeAddr, _config->eventCode())) {
        printf("CspadConfigurator::writeRegs failed on eventCode\n");
        return Failure;
      }
      if (_pgp->writeRegister(&_d, runDelayAddr, _config->runDelay())) {
        printf("CspadConfigurator::writeRegs failed on runDelay\n");
        return Failure;
      }
      if (_pgp->writeRegister(&_d, EvrWidthAddr, EvrWidthValue)) {
        printf("CspadConfigurator::writeRegs failed on EvrWidth\n");
        return Failure;
      }
      unsigned size = MaxQuadsPerSensor*sizeof(ProtectionSystemThreshold)/sizeof(uint32_t);
      if (_pgp->writeRegisterBlock(&_d, protThreshBase, (uint32_t*)_config->protectionThresholds(), size)) {
        printf("CspadConfigurator::writeRegisterBlock failed on protThreshBase\n");
                return Failure;
      }
      if (_pgp->writeRegister(&_d, internalTriggerDelayAddr, _config->internalTriggerDelay())) {
        printf("CspadConfigurator::writeRegs failed on internalTriggerDelayAddr\n");
        return Failure;
      }
      uint32_t d = _config->internalTriggerDelay();
      if (d) {
        d += twoHunderedFiftyMicrosecondsIn8nsIncrements;
        printf(" setting daq trigger delay to %dns", d<<3);
      }
      if (_pgp->writeRegister(&_d, externalTriggerDelayAddr, d)) {
        printf("CspadConfigurator::writeRegs failed on externalTriggerDelayAddr\n");
        return Failure;
      }
      if (_pgp->writeRegister(&_d, ProtEnableAddr, _config->protectionEnable())) {
        printf("CspadConfigurator::writeRegs failed on ProtEnableAddr\n");
        return Failure;
      }
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          u = (uint32_t*)&(_config->quads()[i]);
          _d.dest(i);
          for (unsigned j=0; j<sizeOfQuadWrite; j++) {
            if(_pgp->writeRegister(&_d, _quadAddrs[j], u[j])) {
              printf("CspadConfigurator::writeRegs failed on quad %u address 0x%x\n", i, j);
              return Failure;
            }
          }
        }
      }
      return checkWrittenRegs();
    }

    CspadConfigurator::resultReturn CspadConfigurator::_checkReg(
        CspadDestination* d,
        unsigned     addr,
        unsigned     tid,
        uint32_t     expected) {
      uint32_t readBack;
      if (_pgp->readRegister(d, addr, tid, &readBack)) {
        printf("CspadConfigurator::_checkRegs read back failed at %s %u\n", d->name(), addr);
        return Terminate;
      }
      if (readBack != expected) {
        printf("CspadConfigurator::_checkRegs read back wrong value %u!=%u at %s %u\n",
            expected, readBack, d->name(), addr);
        return Failure;
      }
      return Success;
    }

    unsigned CspadConfigurator::checkWrittenRegs() {
      uint32_t* u;
      unsigned ret = Success;
      unsigned result = Success;
      _d.dest(CspadDestination::CR);
      result |= _checkReg(&_d, EventCodeAddr, 0xa000, _config->eventCode());
      result |= _checkReg(&_d, runDelayAddr, 0xa001, _config->runDelay());
      result |= _checkReg(&_d, EvrWidthAddr, 0xa002, EvrWidthValue);
      result |= _checkReg(&_d, ProtEnableAddr, 0xa003, _config->protectionEnable());
      if (result & Terminate) return Failure;

      ProtectionSystemThreshold pr[MaxQuadsPerSensor];
      unsigned size = MaxQuadsPerSensor*sizeof(ProtectionSystemThreshold)/sizeof(uint32_t);
      if (_pgp->readRegister(&_d, protThreshBase, 0xa004, (uint32_t*)pr, size)){
        printf("CspadConfigurator::writeRegs concentrator read back failed at protThreshBase\n");
        return Failure;
      }
      ProtectionSystemThreshold* pw = _config->protectionThresholds();
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((pw[i].adcThreshold != pr[i].adcThreshold) | (pw[i].pixelCountThreshold != pr[i].pixelCountThreshold)) {
          ret = Failure;
          printf("CspadConfigurator::writeRegs concentrator read back Protection tresholds (%u,%u) != (%u,%u) for quad %u\n",
              pw[i].adcThreshold, pw[i].pixelCountThreshold, pr[i].adcThreshold, pr[i].pixelCountThreshold, i);
        }
      }

      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          _d.dest(i);
          u = (uint32_t*)&(_config->quads()[i]);
          for (unsigned j=0; j<sizeOfQuadWrite; j++) {
            result |= _checkReg(&_d, _quadAddrs[j], 0xb000+(i<<8)+j, u[j]);
          }
        }
      }
      return ret;
    }

    unsigned CspadConfigurator::writeDigPots() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      unsigned ret = Success;
      // size of pots array minus the two in the header plus the NotSupposedToCare, each written as uint32_t.
      unsigned size = Pds::CsPad::PotsPerQuad;
      uint32_t myArray[size];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
      }
      Pds::Pgp::ConfigSynch mySynch(_fd, 0, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor && ret == Success; i++) {
        if ((1<<i) & _config->quadMask()) {
          quadCount += 1;
          _d.dest(i);
          for (unsigned j=0; j<Pds::CsPad::PotsPerQuad; j++) {
           myArray[j] = (uint32_t)_config->quads()[i].dp().value(j);
          }
          if (_pgp->writeRegisterBlock(&_d, _digPot.base, myArray, size) != Success) {
            printf("Writing pots failed\n");
            ret = Failure;
          }
          if (_pgp->writeRegister(
              &_d,
              _digPot.load,
              1,
              false,  // print flag
              quadCount == numberOfQuads ? Pds::Pgp::PgpRSBits::Waiting
                  : Pds::Pgp::PgpRSBits::notWaiting)) {
            printf("Writing load command failed\n");
            ret = Failure;
          }
        }
      }
      if (!mySynch.take()) {
        printf("Write Dig Pots synchronization failed! quadMask(%u), numberOfQuads(%u), quadCount(%u)\n",
            (unsigned)_config->quadMask(), numberOfQuads, quadCount);
        ret = Failure;
      }
      return checkDigPots();
    }

    unsigned CspadConfigurator::checkDigPots() {
      unsigned ret = Success;
      // in uint32_ts, size of pots array
      unsigned size = Pds::CsPad::PotsPerQuad;
      uint32_t myArray[size];
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          _d.dest(i);
          unsigned myRet = _pgp->readRegister(&_d, _digPot.base, 0xc000+(i<<8), myArray,  size);
//          for (unsigned m=0; m<Pds::CsPad::PotsPerQuad; ) {
//            printf("-0x%02x", myArray[m++]);
//            if (!(m&15)) printf("\n");
//          } printf("\n");
          if (myRet == Success) {
            for (unsigned j=0; j<Pds::CsPad::PotsPerQuad; j++) {
              if (myArray[j] != (uint32_t)_config->quads()[i].dp().value(j)) {
                ret = Failure;
                printf("CspadConfigurator::checkDigPots mismatch 0x%02x!=0x%0x at offset %u in quad %u\n",
                    _config->quads()[i].dp().value(j), myArray[j], j, i);
                _config->quads()[i].dp().pots[j] = (uint8_t) (myArray[j] & 0xff);
              }
            }
          } else {
            printf("CspadConfigurator::checkDigPots _pgp->readRegister failed on quad %u\n", i);
            ret = Failure;
          }
        }
      }
      return ret;
    }

    unsigned CspadConfigurator::writeTestData() {
      unsigned ret = Success;
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      unsigned row = 0;
      // number of columns minus the two in the header and plus one for the dnc at the end
      unsigned size = Pds::CsPad::ColumnsPerASIC;
      uint32_t myArray[size];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
      }
      Pds::Pgp::ConfigSynch mySynch(_fd, 0, this, size + 3);  /// !!!!!!!!!!!!!!!!!!!!!!
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          quadCount += 1;
          _d.dest(i);
          for (row=0; row<Pds::CsPad::RowsPerBank; row++) {
            for (unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
              myArray[col] = (uint32_t)rawTestData[_config->tdi()][row][col];
            }
            _pgp->writeRegisterBlock(
                &_d,
                quadTestDataAddr + (row<<8),
                myArray,
                size,
                row == Pds::CsPad::RowsPerBank-1 ?
                                    Pds::Pgp::PgpRSBits::Waiting :
                                    Pds::Pgp::PgpRSBits::notWaiting);
            usleep(MicroSecondsSleepTime);
          }
          if (!mySynch.take()) {
            printf("Write Test Data synchronization failed! i=%u, row=%u, quadCount=%u, numberOfQuads=%u\n", i, row, quadCount, numberOfQuads);
            ret = Failure;
          }
          //        printf(";");
        }// else printf("skipping quad %u\n", i);
      }
      return ret;
    }

    unsigned CspadConfigurator::_internalColWrite(uint32_t val, bool capEn, bool resEn, Pds::Pgp::RegisterSlaveExportFrame* rsef, unsigned col) {
      uint32_t i;
      uint32_t* mem = rsef->array(); // One location per shift register bit
      unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + 16;
      memset(mem,0x0,16*4);
      val >>= 4;  // we want the top four bits
      printf("\n\t_internalColWrite(0x%x, capEn=%5s, resEn=%5s,\t%d\t", val, capEn ? "true" : "false", resEn ? "true" : "false", col);
      // Set value bits, 8 - 11
      for ( i = 0; i < 4; i++ ) // 1 value bit per address. 16-bits = 16 ASICs
         mem[8+i] = (((val >> i) & 0x1) ? 0xFFFF : 0); // all ASICs alike
      // ext cap enable, int res en/ pulsing
      mem[12] = capEn ? 0xFFFF : 0; // all ASICs alike
      mem[13] = resEn ? 0xFFFF : 0;    // all ASICs alike
      if (rsef->post(size)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      if(_pgp->writeRegister(&_d, _gainMap.load, col)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      if (_pgp->writeRegister(&_d, _gainMap.base, 0)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      return Success;
    }

    unsigned CspadConfigurator::_internalColWrite(uint32_t val, bool prstsel, Pds::Pgp::RegisterSlaveExportFrame* rsef, unsigned col) {
      uint32_t i;
      uint32_t* mem = rsef->array(); // One location per shift register bit
      unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + 16;
      memset(mem,0x0,16*4);
      printf("\n\t_internalColWrite(0x%x, prstsel=%5s,\t\t\t%d\t", val, prstsel ? "true" : "false", col);
      // Set value bits, 8 - 15
      for ( i = 0; i < 2; i++ ) { // 1 value bit per address. 16-bits = 16 asics
         mem[i+8]  = (val >> i) &    0x1 ? 0xFFFF : 0x0000; // all asics alike
         mem[i+10] = (val >> i) &   0x10 ? 0xFFFF : 0x0000; // all asics alike
         mem[i+12] = (val >> i) &  0x100 ? 0xFFFF : 0x0000; // all asics alike
         mem[i+14] = (val >> i) & 0x1000 ? 0xFFFF : 0x0000; // all asics alike
      }
      // analog/ digital preset, bit 7
      mem[7] = prstsel ? 0x0000 : 0xFFFF; // all asics alike
      if (rsef->post(size)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      if(_pgp->writeRegister(&_d, _gainMap.load, col)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      if (_pgp->writeRegister(&_d, _gainMap.base, 0)) { return Failure; }
      microSpin(MicroSecondsSleepTime);
      return Success;
    }

    enum {LoopHistoShift=8};

    unsigned CspadConfigurator::writeGainMap() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
      unsigned numberOfQuads = 0;
      Pds::CsPad::CsPadGainMapCfg::GainMap* map[Pds::CsPad::MaxQuadsPerSensor];
      unsigned len = (((Pds::CsPad::RowsPerBank-1)<<3) + Pds::CsPad::BanksPerASIC -1) ;  // there are only 6 banks in the last row
      unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + len + 1 - 2; // last one for the DNC - the two already in the header
      uint32_t myArray[size];
      memset(myArray, 0, size*sizeof(uint32_t));
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
        map[i] = (Pds::CsPad::CsPadGainMapCfg::GainMap*)_config->quads()[i].gm()->map();
      }
      Pds::Pgp::ConfigSynch mySynch(_fd, /*numberOfQuads*/ 1, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
//      printf("\n");
      for (unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
        for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
          if ((1<<i) & _config->quadMask()) {
            _d.dest(i);
            if (!mySynch.take()) {
              printf("Gain Map Write synchronization failed! col(%u), quad(%u)\n", col, i);
              return Failure;
            }
            Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
                Pds::Pgp::PgpRSBits::write,
                &_d,
                _gainMap.base,
                (col << 4) | i,
                (uint32_t)((*map[i])[col][0] & 0xffff),
                Pds::Pgp::PgpRSBits::notWaiting);
            uint32_t* bulk = rsef->array();
            unsigned length=0;
            for (unsigned row=0; row<Pds::CsPad::RowsPerBank; row++) {
              for (unsigned bank=0; bank<Pds::CsPad::BanksPerASIC; bank++) {
                if ((row + bank*Pds::CsPad::RowsPerBank)<Pds::CsPad::MaxRowsPerASIC) {
//                  if (!col) printf("GainMap row(%u) bank(%u) addr(%u)\n", row, bank, (row <<3 ) + bank);
                  bulk[(row <<3 ) + bank] = (uint32_t)((*map[i])[col][bank*Pds::CsPad::RowsPerBank + row] & 0xffff);
                  length += 1;
                }
              }
            }
            bulk[len] = 0;  // write the last word
            //          if ((col==0) && (i==0)) printf(" payload words %u, length %u ", len, len*4);
            rsef->post(size);
            microSpin(MicroSecondsSleepTime);
//            printf("GainMap col(%u) quad(%u) len(%u) length(%u) size(%u)", col, i, len, length, size);
//            for (unsigned m=0; m<len; m++) if (bulk[m] != 0xffff) printf("(%u:%x)", m, bulk[m]);
//            printf("\n");
            if(_pgp->writeRegister(&_d, _gainMap.load, col, false, Pds::Pgp::PgpRSBits::Waiting)) {
              return Failure;
            }
          }
        }
      }
      if (!mySynch.clear()) return Failure;
      for (unsigned i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          _d.dest(i);
          Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::PgpRSBits::write, &_d, _gainMap.base, i);
          unsigned ret = Success;
          uint32_t rce = _config->quads()[i].biasTuning();
          if      (_internalColWrite(_config->quads()[i].dp().pots[iss2addr] & 0xff,      rce &    0x1, rce &    0x2, rsef, 194)) {ret = Failure;}
          else if (_internalColWrite(_config->quads()[i].dp().pots[iss5addr] & 0xff,      rce &   0x10, rce &   0x20, rsef, 195)) {ret = Failure;}
          else if (_internalColWrite(_config->quads()[i].dp().pots[CompBias1addr] & 0xff, rce &  0x100, rce &  0x200, rsef, 196)) {ret = Failure;}
          else if (_internalColWrite(_config->quads()[i].dp().pots[CompBias2addr] & 0xff, rce & 0x1000, rce & 0x2000, rsef, 197)) {ret = Failure;}
          else if (_internalColWrite(_config->quads()[i].pdpmndnmBalance(), _config->quads()[i].prstSel() != 0,       rsef, 198)) {ret = Failure;}
          if (ret) return Failure;
        }
      }
      return Success;
    }
  } // namespace CsPad
} // namespace Pds

