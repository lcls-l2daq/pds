/*
 * Cspad2x2Configurator.cc
 *
 *  Created on: Jan 9, 2012
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
#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pdsdata/cspad2x2/Detector.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"

#include "TestData.cc"

namespace Pds {
  namespace CsPad2x2 {

    class Cspad2x2QuadRegisters;
    class Cspad2x2ConcentratorRegisters;
    class Cspad2x2Destination;
    class ProtectionSystemThreshold;
    class Pds::Pgp::AddressRange;

//    --    0x700000 : PeltierEnable, 1-bit  -> 0 (off)
//    --    0x700001 : kpConstant, 12-bit -> 100
//    --    0x700002 : kiConstant, 8-bit -> 0
//    --    0x700003 : kdConstant, 8-bit -> 0
//    --    0x700004 : humidThold, 12-bit -> 0
//    --    0x700005 : setPoint,   12-bit -> 20 deg C
//    --    0x700006 : dacValue,   16-bit (status read only)

    unsigned                  Cspad2x2Configurator::_quadAddrs[] = {
        0x110001,  // shiftSelect0    1
        0x110002,  // edgeSelect0     2
        0x110003,  // readClkSet      3
        0x110005,  // readClkHold     4
        0x110006,  // dataMode        5
        0x300000,  // prstSel         6
        0x300001,  // acqDelay        7
        0x300002,  // intTime         8
        0x300003,  // digDelay        9
        0x300005,  // ampIdle        10
        0x300006,  // injTotal       11
        0x400001,  // rowColShiftPer 12
        0x300009,  // ampReset       13
        0x30000b,  // digCount       14
        0x30000a,  // digPeriod      15
        0x700000,  // PeltierEnable  16
        0x700001,  // kpConstant     17
        0x700002,  // kiConstant     18
        0x700003,  // kdConstant     19
        0x700004,  // humidThold     20
        0x700005   // setPoint       21     sizeOfQuadWrite

    };

    unsigned                   Cspad2x2Configurator::_quadReadOnlyAddrs[] = {
        0x400000,  // shiftTest       1
        0x500000   // version         2    sizeOfQuadReadOnly
    };

    enum {sizeOfQuadWrite=21, sizeOfQuadReadOnly=2};

    Cspad2x2Configurator::Cspad2x2Configurator( CsPad2x2ConfigType* c, int f, unsigned d) :
                   Pds::Pgp::Configurator(f, d),
                   _config(c), _rhisto(0),
                   _conRegs(new Cspad2x2ConcentratorRegisters()),
                   _quadRegs(new Cspad2x2QuadRegisters()) {
      Cspad2x2ConcentratorRegisters::configurator(this);
      Cspad2x2QuadRegisters::configurator(this);
      _initRanges();
      strcpy(_runTimeConfigFileName, "");
      printf("Cspad2x2Configurator constructor _config(%p)\n", _config);
      //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
      //    _rhisto = (unsigned*) calloc(1000, 4);
      //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
    }

    Cspad2x2Configurator::~Cspad2x2Configurator() {
      delete (_conRegs);
      delete (_quadRegs);
    }

    void Cspad2x2Configurator::print() {}

    void Cspad2x2Configurator::printMe() {
      printf("Configurator: ");
      for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
      printf("\n");
    }

    void Cspad2x2Configurator::runTimeConfigName(char* name) {
      if (name) strcpy(_runTimeConfigFileName, name);
      printf("Cspad2x2Configurator::runTimeConfigName(%s)\n", name);
    }

    void Cspad2x2Configurator::_initRanges() {
      new ((void*)&_gainMap) Pds::Pgp::AddressRange(0x000000, 0x010000);
      new ((void*)&_digPot)  Pds::Pgp::AddressRange(0x200000, 0x210000);
    }

    bool Cspad2x2Configurator::_flush(unsigned index) {
      enum {numberOfTries=5};
      unsigned version = 0;
      unsigned failCount = 0;
      bool ret = false;
      printf("\n--flush-%u-", index);
      _d.dest(Cspad2x2Destination::CR);
      while ((Failure == _pgp->readRegister(
          &_d,
          ConcentratorVersionAddr,
          0x55000,
          &version)) && failCount++<numberOfTries ) {
        printf("%s(%u)\n", _d.name(), failCount);
      }
      if (failCount<numberOfTries) printf("\t%s(0x%x)\n", _d.name(), version);
      else ret = true;
      version = 0;
      failCount = 0;
      _d.dest(Cspad2x2Destination::Q0);
      while ((Failure == _pgp->readRegister(
          &_d,
          0x500000,  // version address
          0x50000,
          &version)) && failCount++<numberOfTries ) {
        printf("[%s:%u]\n", _d.name(), failCount);
      }
      if (failCount<numberOfTries) printf("\t%s[0x%x]\n", _d.name(), version);
      else ret = true;
      return ret;
    }

    unsigned Cspad2x2Configurator::configure(unsigned mask) {
      timespec      start, end, sleepTime, shortSleepTime;
      sleepTime.tv_sec = 0;
      sleepTime.tv_nsec = 25000000; // 25ms
      shortSleepTime.tv_sec = 0;
      shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
      bool printFlag = !(mask & 0x2000);
      if (printFlag) printf("Cspad2x2 Config");
      printf(" config(%p) mask(0x%x)\n", &_config, ~mask);
      unsigned ret = 0;
      mask = ~mask;
      clock_gettime(CLOCK_REALTIME, &start);
      // N.B. We MUST stop running before resetting anything, preventing garbaging the FE pipeline
      _d.dest(Cspad2x2Destination::CR);
      ret |= _pgp->writeRegister(&_d, RunModeAddr, Pds::CsPad2x2::NoRunning);
      nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
      //        ret |= _pgp->writeRegister(&_d, resetQuadsAddr, 1);
//      clock_gettime(CLOCK_REALTIME, &end); printf("\n\ttxClock %d %ld\n", (int)end.tv_sec, end.tv_nsec);
      _pgp->writeRegister(&_d, resetAddr, 1);
      nanosleep(&sleepTime, 0); //nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
      if (_flush(0)) printf("Cspad2x2Configurator::configure _flush(0) FAILED\n");
      ret |= _pgp->writeRegister(&_d, TriggerWidthAddr, TriggerWidthValue);
      ret |= _pgp->writeRegister(&_d, ResetSeqCountRegisterAddr, 1);
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
      if (mask&4 && ret==0) {
        if (printFlag) printf("- 0x%x - \n\twriting test data[%u] ", ret, (unsigned)_config->tdi());
        ret |= writeTestData();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
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
      if (_flush(1)) printf("Cspad2x2Configurator::configure _flush(1) FAILED\n");
      loadRunTimeConfigAdditions(_runTimeConfigFileName);
      if (mask&16 && ret==0) {
        if (printFlag) printf("- 0x%x - \n\treading ", ret);
        ret |= readRegs();
        if (printFlag) {
          clock_gettime(CLOCK_REALTIME, &end);
          uint64_t diff = timeDiff(&end, &start) + 50000LL;
          printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
        }
      }
      ret <<= 1;
      _d.dest(Cspad2x2Destination::CR);
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
        //    print();
//        if (ret) dumpFrontEnd();
      }
      return ret;
    }

    void Cspad2x2Configurator::dumpFrontEnd() {
      timespec      start, end;
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned ret = Success;
      if (_debug & 0x100) {
        printf("Cspad2x2Configurator::dumpFrontEnd: Concentrator\n");
        ret = _conRegs->read();
        if (ret == Success) {
          _conRegs->print();
        }
      }
      if ((_debug & 0x200) && (ret == Success)) {
        printf("Cspad2x2Configurator::dumpFrontEnd: Quad\n");
        ret = _quadRegs->read();
        if (ret == Success) {
          _quadRegs->print();
        }
      } if (ret != Success) {
        printf("\tCould not be read!\n");
      }
      if ((_debug & 0x400) && (ret == Success)) {
        printf("Checking Configuration, no news is good news ...\n");
        if (Failure == checkWrittenRegs()) {
          printf("Cspad2x2Configurator::checkWrittenRegs() FAILED !!!\n");
        }
        if (Failure == checkDigPots()) {
          printf("Cspad2x2Configurator::checkDigPots() FAILED !!!!\n");
        }
      }
      if ((_debug & 0x800) && (ret == Success)) {
        dumpPgpCard();
      }
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      if (_debug & 0x700) {
        printf("Cspad2x2Configurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
        printf(" - %s\n", ret == Success ? "Success" : "Failed!");
      }
      return;
    }

    unsigned Cspad2x2Configurator::readRegs() {
      unsigned ret = Success;
      _d.dest(Cspad2x2Destination::Q0);
      uint32_t* u = (uint32_t*) _config->quad()->readOnly();
       for (unsigned j=0; j<sizeOfQuadReadOnly; j++) {
        if (Failure == _pgp->readRegister(&_d, _quadReadOnlyAddrs[j], 0x55000 | j, u+j)) {
          ret = Failure;
        }
      }
      if (_debug & 0x10) printf("Cspad2x2Configurator Quad read only 0x%x 0x%x %p %p\n",
          (unsigned)u[0], (unsigned)u[1], u, _config->quad()->readOnly());
      _d.dest(Cspad2x2Destination::CR);
     if (Failure == _pgp->readRegister(
          &_d,
          ConcentratorVersionAddr,
          0x55000,
          _config->concentratorVersionAddr())) {
        ret = Failure;
      }
      if (_debug & 0x10) printf("\nCspad2x2Configurator concentrator version 0x%x\n", *_config->concentratorVersionAddr());
      return ret;
    }

    unsigned Cspad2x2Configurator::writeRegs() {
      uint32_t* u;
      _d.dest(Cspad2x2Destination::CR);
      unsigned size = QuadsPerSensor*sizeof(ProtectionSystemThreshold)/sizeof(uint32_t);
      if (_pgp->writeRegisterBlock(&_d, protThreshBase, (uint32_t*)_config->protectionThreshold(), size)) {
        printf("Cspad2x2Configurator::writeRegisterBlock failed on protThreshBase\n");
                return Failure;
      }
      if (_pgp->writeRegister(&_d, ProtEnableAddr, _config->protectionEnable())) {
        printf("Cspad2x2Configurator::writeRegs failed on ProtEnableAddr\n");
        return Failure;
      }
      u = (uint32_t*)(_config->quad());
      _d.dest(Cspad2x2Destination::Q0);
      for (unsigned j=0; j<sizeOfQuadWrite; j++) {
        if(_pgp->writeRegister(&_d, _quadAddrs[j], u[j])) {
          printf("Cspad2x2Configurator::writeRegs failed on quad address 0x%x\n", j);
          return Failure;
        }
      }
      return checkWrittenRegs();
    }

    Cspad2x2Configurator::resultReturn Cspad2x2Configurator::_checkReg(
        Cspad2x2Destination* d,
        unsigned     addr,
        unsigned     tid,
        uint32_t     expected) {
      uint32_t readBack;
      if (_pgp->readRegister(d, addr, tid, &readBack)) {
        printf("Cspad2x2Configurator::_checkReg read back failed at %s address %u\n", d->name(), addr);
        return Terminate;
      }
      if (readBack != expected) {
        printf("Cspad2x2Configurator::_checkReg read back wrong value %u!=%u at %s address 0x%x\n",
            expected, readBack, d->name(), addr);
        return Failure;
      }
      return Success;
    }

    unsigned Cspad2x2Configurator::checkWrittenRegs() {
      uint32_t* u;
      unsigned ret = Success;
      unsigned result = Success;
      _d.dest(Cspad2x2Destination::CR);
      result |= _checkReg(&_d, ProtEnableAddr, 0xa003, _config->protectionEnable());
      if (result & Terminate) return Failure;

      ProtectionSystemThreshold pr;
      unsigned size = sizeof(ProtectionSystemThreshold)/sizeof(uint32_t);
      if (_pgp->readRegister(&_d, protThreshBase, 0xa004, (uint32_t*)&pr, size)){
        printf("Cspad2x2Configurator::writeRegs concentrator read back failed at protThreshBase\n");
        return Failure;
      }
      ProtectionSystemThreshold* pw = _config->protectionThreshold();
      for (unsigned i=0; i<QuadsPerSensor; i++) {
        if ((pw->adcThreshold != pr.adcThreshold) | (pw->pixelCountThreshold != pr.pixelCountThreshold)) {
          ret = Failure;
          printf("Cspad2x2Configurator::writeRegs concentrator read back Protection threshold (%u,%u) != (%u,%u)\n",
              pw->adcThreshold, pw->pixelCountThreshold, pr.adcThreshold, pr.pixelCountThreshold);
        }
      }
      _d.dest(Cspad2x2Destination::Q0);
      u = (uint32_t*)(_config->quad());
      for (unsigned j=0; j<sizeOfQuadWrite; j++) {
//        printf("checkWrittenRegs reguesting address 0x%x from %s at index %u\n", _quadAddrs[j], _d.name(), j);
        result |= _checkReg(&_d, _quadAddrs[j], 0xb000+j, u[j]);
      }
      return ret;
    }

    unsigned Cspad2x2Configurator::writeDigPots() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
      unsigned ret = Success;
      // size of pots array minus the two in the header plus the NotSupposedToCare, each written as uint32_t.
      unsigned size = Pds::CsPad2x2::PotsPerQuad;
      uint32_t myArray[size];
      Pds::Pgp::ConfigSynch mySynch(_fd, 0, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
      _d.dest(Cspad2x2Destination::Q0);
      for (unsigned j=0; j<Pds::CsPad2x2::PotsPerQuad; j++) {
        myArray[j] = (uint32_t)_config->quad()->dp().value(j);
      }
      if (_pgp->writeRegisterBlock(&_d, _digPot.base, myArray, size) != Success) {
        printf("Writing quad pots failed\n");
        ret = Failure;
      }
      if (_pgp->writeRegister(
          &_d,
          _digPot.load,
          1,
          false,  // print flag
          Pds::Pgp::PgpRSBits::Waiting)) {
        printf("Writing quad pots load command failed\n");
        ret = Failure;
      }
      if (!mySynch.take()) {
        printf("Write Dig Pots synchronization failed!\n");
        ret = Failure;
      }
      return checkDigPots();
    }

    unsigned Cspad2x2Configurator::checkDigPots() {
      unsigned ret = Success;
      // in uint32_ts, size of pots array
      unsigned size = Pds::CsPad2x2::PotsPerQuad;
      uint32_t myArray[size];
      _d.dest(Cspad2x2Destination::Q0);
      unsigned myRet = _pgp->readRegister(&_d, _digPot.base, 0xc000, myArray,  size);
      //          for (unsigned m=0; m<Pds::CsPad2x2::PotsPerQuad; ) {
      //            printf("-0x%02x", myArray[m++]);
      //            if (!(m&15)) printf("\n");
      //          } printf("\n");
      if (myRet == Success) {
        for (unsigned j=0; j<Pds::CsPad2x2::PotsPerQuad; j++) {
          if (myArray[j] != (uint32_t)_config->quad()->dp().value(j)) {
            ret = Failure;
            printf("Cspad2x2Configurator::checkDigPots mismatch 0x%02x!=0x%0x at offset %u in quad\n",
                _config->quad()->dp().value(j), myArray[j], j);
            _config->quad()->dp().pots[j] = (uint8_t) (myArray[j] & 0xff);
          }
        }
      } else {
        printf("Cspad2x2Configurator::checkDigPots _pgp->readRegister failed on quad\n");
        ret = Failure;
      }
      return ret;
    }

    unsigned Cspad2x2Configurator::writeTestData() {
      unsigned ret = Success;
      unsigned row;
      unsigned size = Pds::CsPad2x2::ColumnsPerASIC;
      uint32_t myArray[size];
      Pds::Pgp::ConfigSynch mySynch(_fd, 0, this, size + 3);  /// !!!!!!!!!!!!!!!!!!!!!!
      _d.dest(Cspad2x2Destination::Q0);
      for (row=0; row<Pds::CsPad2x2::RowsPerBank; row++) {
        for (unsigned col=0; col<Pds::CsPad2x2::ColumnsPerASIC; col++) {
          myArray[col] = (uint32_t)rawTestData[_config->tdi()][row][col];
        }
        _pgp->writeRegisterBlock(
            &_d,
            quadTestDataAddr + (row<<8),
            myArray,
            size,
            row == Pds::CsPad2x2::RowsPerBank-1 ?
                Pds::Pgp::PgpRSBits::Waiting :
        Pds::Pgp::PgpRSBits::notWaiting);
        usleep(MicroSecondsSleepTime);
      }
      if (!mySynch.take()) {
        printf("Write Test Data synchronization failed! row=%u\n", row);
        ret = Failure;
      }
      return ret;
    }

    enum {LoopHistoShift=8};

    unsigned Cspad2x2Configurator::writeGainMap() {
      Pds::CsPad2x2::CsPad2x2GainMapCfg::GainMap* map[1];
      unsigned len = (((Pds::CsPad2x2::RowsPerBank-1)<<3) + Pds::CsPad2x2::BanksPerASIC -1) ;  // there are only 6 banks in the last row
      unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + len + 1 - 2; // last one for the DNC - the two already in the header
      uint32_t myArray[size];
      memset(myArray, 0, size*sizeof(uint32_t));
      map[0] = (Pds::CsPad2x2::CsPad2x2GainMapCfg::GainMap*)_config->quad()->gm()->map();
      Pds::Pgp::ConfigSynch mySynch(_fd, QuadsPerSensor, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
//      printf("\n");
      for (unsigned col=0; col<Pds::CsPad2x2::ColumnsPerASIC; col++) {
        _d.dest(Cspad2x2Destination::Q0);
        if (!mySynch.take()) {
          printf("Gain Map Write synchronization failed! col(%u)\n", col);
          return Failure;
        }
        Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
            Pds::Pgp::PgpRSBits::write,
            &_d,
            _gainMap.base,
            (col << 4),
            (uint32_t)((*map[0])[col][0] & 0xffff),
            Pds::Pgp::PgpRSBits::notWaiting);
        uint32_t* bulk = rsef->array();
        unsigned length=0;
        for (unsigned row=0; row<Pds::CsPad2x2::RowsPerBank; row++) {
          for (unsigned bank=0; bank<Pds::CsPad2x2::BanksPerASIC; bank++) {
            if ((row + bank*Pds::CsPad2x2::RowsPerBank)<Pds::CsPad2x2::MaxRowsPerASIC) {
              //                  if (!col) printf("GainMap row(%u) bank(%u) addr(%u)\n", row, bank, (row <<3 ) + bank);
              bulk[(row <<3 ) + bank] = (uint32_t)((*map[0])[col][bank*Pds::CsPad2x2::RowsPerBank + row] & 0xffff);
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
      if (!mySynch.clear()) return Failure;
      return Success;
    }
  } // namespace CsPad2x2
} // namespace Pds
