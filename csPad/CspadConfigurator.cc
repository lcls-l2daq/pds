/*
 * CspadConfigurator.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "pds/csPad/CspadConfigurator.hh"
//#include "csPad/CspadReadOnlyRegisters.hh"
//#include "rceusr/pgptest/PgpStat.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/config/CsPadConfigType.hh"
//#include <rtems.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "PgpCardMod.h"

#include "TestData.cc"

namespace Pds {

  namespace CsPad {

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
        0x400001  // rowColShiftPer  18
    };

    unsigned                   CspadConfigurator::_quadReadOnlyAddrs[] = {
        0x400000,  // shiftTest       1
        0x500000   // version         2
    };

    CspadConfigurator::CspadConfigurator( CsPadConfigType& c, int f) :
                   _config(c), _fd(f), _rhisto(0), _print(false) {
      _initRanges();
      printf("CspadConfigurator constructor _config(%p), quadMask(0x%x)\n",
         &_config, (unsigned)_config.quadMask());
      //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
      //    _rhisto = (unsigned*) calloc(1000, 4);
      //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
    }

    void CspadConfigurator::printMe() {
      printf("Configurator: ");
      for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
      printf("\n");
    }

    long long int CspadConfigurator::timeDiff(timespec* end, timespec* start) {
      long long int diff;
      diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
      diff += end->tv_nsec;
      diff -= start->tv_nsec;
      return diff;
    }

    unsigned CspadConfigurator::configure(unsigned mask) {
      timespec      start, end, sleepTime;
      sleepTime.tv_sec = 0;
      sleepTime.tv_nsec = 50000000; // 50ms
      bool printFlag = !(mask & 0x2000);
      if (printFlag) printf("CXI Config");
      printf(" config(%p) quadMask(0x%x) ", &_config, (unsigned)_config.quadMask());
      unsigned ret = 0;
      mask = ~mask;
      //    _print = true;
      clock_gettime(CLOCK_REALTIME, &start);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, Pds::CsPad::NoRunning);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetQuadsAddr, 1);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetAddr, 1);
      nanosleep(&sleepTime, 0);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, Pds::CsPad::NoRunning);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, TriggerWidthAddr, TriggerWidthValue);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, ResetSeqCountRegisterAddr, 1);
      _print = false;
      ret <<= 1;
      if (mask&1) {
        if (printFlag) printf("\nConfiguring:\n\twriting control regs - 0x%x - \n\twriting regs", ret);
        ret = writeRegs();
      }
      ret <<= 1;
      if (mask&2) {
        if (printFlag) printf(" - 0x%x - \n\twriting pots ", ret);
        ret |= writeDigPots();
      }
      ret <<= 1;
      if (mask&4) {
        if (printFlag) printf("- 0x%x - \n\twriting test data[%u] ", ret, (unsigned)_config.tdi());
        ret |= writeTestData();
      }
      ret <<= 1;
      if (mask&8) {
        if (printFlag) printf("- 0x%x - \n\twriting gain map ", ret);
        ret |= writeGainMap();
      }
      ret <<= 1;
      if (mask&16) {
        if (printFlag) printf("- 0x%x - \n\treading ", ret);
        ret |= read();
      }
      //    _print = true;
      ret <<= 1;
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, EnableEvrAddr, EnableEvrValue);
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, _config.inactiveRunMode());
      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetCountersAddr, 1);
      _print = false;
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - \n\tdone \n", ret);
        printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
        //      printf(" quadMask(0x%x) ", (unsigned)_config.quadMask());
        //    print();
      }
      return ret;
    }

    unsigned CspadConfigurator::writeRegister(
        Pds::Pgp::RegisterSlaveExportFrame::FEdest dst,
        unsigned addr,
        uint32_t data,
        Pds::Pgp::RegisterSlaveExportFrame::waitState w) {
      Pds::Pgp::RegisterSlaveExportFrame* rsef;
      rsef = new Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
          Pds::Pgp::RegisterSlaveExportFrame::write,
          dst,
          addr,
          6969,
          data,
          w);
      if (_print) rsef->print();
      return rsef->post(sizeof(rsef));
    }

    unsigned CspadConfigurator::writeRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest dst, unsigned addr, uint32_t data, bool printFlag) {
      _print = printFlag;
      unsigned ret = writeRegister(dst, addr, data, Pds::Pgp::RegisterSlaveExportFrame::notWaiting);
      _print = false;
      return ret;
    }

    unsigned CspadConfigurator::readRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest dest, unsigned addr, unsigned tid, uint32_t* retp) {
      Pds::Pgp::RegisterSlaveImportFrame rsif;
      fd_set          fds;
      struct timeval  timeout;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;
      PgpCardRx       pgpCardRx;
      Pds::Pgp::RegisterSlaveExportFrame* rsef;
      rsef = new Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
          Pds::Pgp::RegisterSlaveExportFrame::read,
          dest,
          addr,
          tid,
          (uint32_t)0,
          Pds::Pgp::RegisterSlaveExportFrame::Waiting);
      if (!rsef->post(sizeof(rsef))) {
        printf("CspadConfigurator::readRegister failed, export frame follows.\n");
        rsef->print();
        return  Failure;
      }
      unsigned errorCount = 0;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = sizeof(rsif) / sizeof(__u32);
      pgpCardRx.data    = (__u32*)&rsif;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);
      while (true) {
        if ( select( _fd+1, &fds, NULL, NULL, &timeout) > 0) {
          ::read(_fd, &pgpCardRx, sizeof(PgpCardRx));
        } else {
          perror("CspadConfigurator::readRegister select call: ");
          return Failure;
        }
        if (addr != rsif.addr()) {
          printf("\nReadConfig out of order response dest=%u, addr=0x%x, tid=%u, errorCount=%u\n",
              dest, addr, tid, ++errorCount);
          rsif.print();
          if (errorCount > 5) return Failure;
        } else {
          *retp = rsif.data();
        } return Success;
      }
    }

    unsigned CspadConfigurator::read() {
      if (Failure == readRegister(
          Pds::Pgp::RegisterSlaveExportFrame::CR,
          ConcentratorVersionAddr,
          0,
          _config.concentratorVersionAddr())) {
        return Failure;
      }
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) {
          uint32_t* u = (uint32_t*) _config.quads()[i].readOnly();
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          for (unsigned j=0; j<sizeOfQuadReadOnly; j++) {
            if (Failure == readRegister(dest, _quadReadOnlyAddrs[j], i*sizeOfQuadReadOnly + j, u+j)) {
              return Failure;
            }
          }
        }
      }
      return Success;
    }

    unsigned CspadConfigurator::writeRegs() {
      uint32_t* u;
      if (writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, EventCodeAddr, _config.eventCode())) {
        return Failure;
      }
      //    printf(" ev code %u", (unsigned)_config.eventCode());
      if (writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, runDelayAddr, _config.runDelay())) {
        return Failure;
      }
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) {
          u = (uint32_t*)&(_config.quads()[i]);
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          for (unsigned j=0; j<sizeOfQuadWrite; j++) {
            if(writeRegister(dest, _quadAddrs[j], u[j])) {
              return Failure;
            }
          }
        }
      }
      return Success;
    }

    unsigned CspadConfigSynch::_getOne() {
      Pds::Pgp::RegisterSlaveImportFrame rsif;
      fd_set          fds;
      struct timeval  timeout;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;
      PgpCardRx       pgpCardRx;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = sizeof(rsif) / sizeof(__u32);
      pgpCardRx.data    = (__u32*)&rsif;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);

      if (select(_fd+1,&fds,NULL,NULL,&timeout) > 0) {
        ::read(_fd,&pgpCardRx,sizeof(PgpCardRx));
      } else {
        printf("Cspad Config Synchronizer receive select timed out\n");
        return Failure;
      }

      if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
        printf("Cspad Config Synchronizer pgpcard error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
            pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
        return Failure;
      }

      if (rsif.failed()) {
        printf("Cspad Config Synchronizer receive HW failed\n");
        rsif.print();
        return Failure;
      }

      if (rsif.timeout()) {
        printf("Cspad Config Synchronizer receive HW timed out\n");
        rsif.print();
        return Failure;
      }
      return Success;
    }

    bool CspadConfigSynch::take() {
      if (_depth == 0) {
        if (_getOne()) {
          return false;
        }
      } else {
        _depth -= 1;
      }
      return true;
    }

    bool CspadConfigSynch::clear() {
      int cnt = _length;
      while (cnt) {
        //      printf("/nClearing %d ", _length-cnt);
        if (_getOne()) {
          printf("Cspad Config Synchronizer receive clearing failed, missing %d\n", cnt);
          return false;
        }
        cnt--;
      }
      return true;
    }

    unsigned CspadConfigurator::writeDigPots() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config.quadMask());
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      // in bytes, size of pots array minus the two in the header plus the NotSupposedToCare, written as uint32_t.
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + PotsPerQuad -2 + 1];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) numberOfQuads += 1;
      }
      CspadConfigSynch mySynch(_fd, 0, _rhisto);
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) {
          quadCount += 1;
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          //          printf("\n/tpot write, %u, 0x%x, %u", j, (unsigned)(_digPot.base + j), (unsigned)_config.quads()[i].dp().value(j));
          Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::RegisterSlaveExportFrame::write,
              dest,
              _digPot.base,
              i);
          uint32_t* bulk = rsef->array();
          for (unsigned j=0; j<PotsPerQuad; j++) {
            bulk[j] = (uint32_t)_config.quads()[i].dp().value(j);
          }
          bulk[PotsPerQuad] = 0;
          if (rsef->post(sizeof(myArray) != Success)) {
            printf("Writing pots failed\n");
          }
          if (writeRegister(dest,
              _digPot.load,
              1,
              quadCount == numberOfQuads ? Pds::Pgp::RegisterSlaveExportFrame::Waiting
                  : Pds::Pgp::RegisterSlaveExportFrame::notWaiting)) {
            printf("Writing load command failed\n");
            return Failure;
          }
          //        printf(";");
        }// else printf("skipping quad %u\n", i);
      }
      if (!mySynch.take()) {
        printf("Write Dig Pots synchronization failed! quadMask(%u), numberOfQuads(%u), quadCount(%u)\n",
            (unsigned)_config.quadMask(), numberOfQuads, quadCount);
        return Failure;
      }
      //    printf("\n");
      return Success;
    }

    unsigned CspadConfigurator::writeTestData() {
      unsigned ret = Success;
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) +  ColumnsPerASIC - 2 + 1];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) numberOfQuads += 1;
      }
      CspadConfigSynch mySynch(_fd, 0, _rhisto);
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) {
          quadCount += 1;
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          for (unsigned row=0; row<RowsPerBank; row++) {
            Pds::Pgp::RegisterSlaveExportFrame* rsef;
            rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
                Pds::Pgp::RegisterSlaveExportFrame::write,
                dest,
                quadTestDataAddr + (row<<8),  // hardware wants rows spaced 8 bits apart
                row + i*RowsPerBank,
                0,
                (row == RowsPerBank-1) && (quadCount == numberOfQuads) ?
                    Pds::Pgp::RegisterSlaveExportFrame::Waiting : Pds::Pgp::RegisterSlaveExportFrame::notWaiting);
            uint32_t* bulk = rsef->array();
            for (unsigned col=0; col<ColumnsPerASIC; col++) {
              bulk[col] = (uint32_t)rawTestData[_config.tdi()][row][col];
            }
            bulk[ColumnsPerASIC] = 0;
            // number of columns minus the two in the header and plus one for the dnc at the end
            rsef->post(sizeof(myArray));
          }
          //        printf(";");
        }// else printf("skipping quad %u\n", i);
      }
      if (!mySynch.take()) {
        printf("Write Test Data synchronization failed! quadMask(%u), numberOfQuads(%u), quadCount(%u)\n",
            (unsigned)_config.quadMask(), numberOfQuads, quadCount);
        ret = Failure;
      }
      return ret;
    }

    enum {LoopHistoShift=8};

    void CspadConfigurator::print() {}

    unsigned CspadConfigurator::writeGainMap() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config.quadMask());
      unsigned numberOfQuads = 0;
      Pds::CsPad::CsPadGainMapCfg::GainMap* map[Pds::CsPad::MaxQuadsPerSensor];
      // the first two are in the header, but we need one DNC for Ryan's logic
      //    one plus the address of the last word minus the two in the header plus the dnc.
      unsigned len = (((RowsPerBank-1)<<3) + BanksPerASIC-2 + 1) - 2 + 1;  // uint32_t's in the payload
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + len];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config.quadMask()) numberOfQuads += 1;
        map[i] = (Pds::CsPad::CsPadGainMapCfg::GainMap*)_config.quads()[i].gm()->map();
      }
      CspadConfigSynch mySynch(_fd, numberOfQuads, _rhisto);
      for (unsigned col=0; col<ColumnsPerASIC; col++) {
        for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
          if ((1<<i) & _config.quadMask()) {
            Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
            if (!mySynch.take()) {
              printf("Gain Map Write synchronization failed! col(%u), quad(%u)\n", col, i);
              return Failure;
            }
            Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
                Pds::Pgp::RegisterSlaveExportFrame::write,
                dest,
                _gainMap.base,
                col + i * ColumnsPerASIC,
                (uint32_t)((*map[i])[col][0] & 0xffff),
                Pds::Pgp::RegisterSlaveExportFrame::notWaiting);
            uint32_t* bulk = rsef->array();
            for (unsigned row=0; row<RowsPerBank; row++) {
              for (unsigned bank=0; bank<BanksPerASIC; bank++) {
                if ((row + bank*RowsPerBank)<MaxRowsPerASIC) {
                  bulk[(row <<3 ) + bank] = (uint32_t)((*map[i])[col][bank*RowsPerBank + row] & 0xffff);
                }
              }
            }
            bulk[len] = 0;
            //          if ((col==0) && (i==0)) printf(" payload words %u, length %u ", len, len*4);
            rsef->post(sizeof(myArray));

            if(writeRegister(dest, _gainMap.load, col, Pds::Pgp::RegisterSlaveExportFrame::Waiting)) {
              return Failure;
            }
          }
        }
      }
      if (!mySynch.clear()) return Failure;
      return Success;
    }
  }
}

