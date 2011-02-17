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
#include "PgpCardMod.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/csPad/CspadConfigurator.hh"
//#include "csPad/CspadReadOnlyRegisters.hh"
//#include "rceusr/pgptest/PgpStat.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/config/CsPadConfigType.hh"

#include "TestData.cc"

namespace Pds {
  namespace CsPad {

    void* rxMain(void* p);
    class CspadQuadRegisters;
    class CspadConcentratorRegisters;
    class CspadDirectRegisterReader;

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

    char                       CspadConfigurator::_inputQueueName[] = "/MsgFromCsPadReceiverQueue";
    char                       CspadConfigurator::_outputQueueName[] = "/MsgToCsPadReceiverQueue";

    CspadConfigurator::CspadConfigurator( CsPadConfigType* c, int f, unsigned d) :
                   _config(c), _fd(f), _rhisto(0), _rxThread(0), _debug(d),
                   _drdr(new CspadDirectRegisterReader(f)), _conRegs(new CspadConcentratorRegisters()),
                   _quadRegs(new CspadQuadRegisters()), _print(false) {
      _initRanges();
       printf("CspadConfigurator constructor _config(%p), quadMask(0x%x)\n",
         _config, (unsigned)_config->quadMask());
      //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
      //    _rhisto = (unsigned*) calloc(1000, 4);
      //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
    }

    CspadConfigurator::~CspadConfigurator() {
      printf("CspadConfigurator destructor unlinking queues\n");
      mq_unlink(_inputQueueName);
      mq_unlink(_outputQueueName);
      delete (_drdr);
      delete (_conRegs);
      delete (_quadRegs);
    }

    void CspadConfigurator::printMe() {
      printf("Configurator: ");
      for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
      printf("\n");
    }

#define PERMS (S_IRUSR|S_IRUSR|S_IRUSR|S_IROTH|S_IROTH|S_IROTH|S_IRGRP|S_IRGRP|S_IRGRP| \
    S_IWUSR|S_IWUSR|S_IWUSR|S_IWOTH|S_IWOTH|S_IWOTH|S_IWGRP|S_IWGRP|S_IWGRP)
#define OFLAGS (O_CREAT|O_RDWR)

    enum MSG_TYPE {Enable, EnableAck, Disable, DisableAck, PGPrsif};
    enum {MAGIC=0xFEEDFACE};

    class Msg {
      public:
        Msg() {magick = MAGIC;}
        ~Msg() {}
        unsigned magick;
        enum MSG_TYPE type;
        Pds::Pgp::RegisterSlaveImportFrame* rsif;
    } myOut, myIn;

    long long int CspadConfigurator::timeDiff(timespec* end, timespec* start) {
      long long int diff;
      diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
      diff += end->tv_nsec;
      diff -= start->tv_nsec;
      return diff;
    }

    void CspadConfigurator::microSpin(unsigned m) {
      long long unsigned gap = 0;
      timespec start, now;
      clock_gettime(CLOCK_REALTIME, &start);
      while (gap < m) {
        clock_gettime(CLOCK_REALTIME, &now);
        gap = timeDiff(&now, &start) / 1000LL;
      }
    }

    bool CspadConfigurator::_startRxThread() {
      bool ret = true;
      unsigned      priority = 0;
      Msg _myIn, _myOut;
      struct mq_attr _mymq_attr;
      _mymq_attr.mq_maxmsg = 10L;
      _mymq_attr.mq_msgsize = (long int)sizeof(_myIn);
      _mymq_attr.mq_flags = 0L;
      printf("1");
      if (_rxThread == 0) {
        umask(0);
        printf("2");
        _myOutputQueue = mq_open(_outputQueueName, OFLAGS, PERMS, &_mymq_attr);
        if (_myInputQueue < 0) {
          perror("CspadConfigurator::_startRxThread mq_open output to receiver thread");
          ret = false;
        }
        _myInputQueue = mq_open(_inputQueueName, OFLAGS, PERMS, &_mymq_attr);
        if (_myOutputQueue < 0) {
          perror("CspadConfigurator::_startRxThread mq_open input from  receiver thread");
          ret = false;
        }
         if ( pthread_create( &_rxThread, NULL, rxMain, this) < 0 ) {
          perror("CspadConfigurator::_startRxThread Error creating receiver thread\n");
          ret = false;
        } else {
          printf("spadConfigurator::_startRxThread created thread _rxThread(0x%x)\n", (unsigned)_rxThread);
        }
      }
      printf("CspadConfigurator::_startRxThread _myInputQueue(%d)  _myOutputQueue(%d) _rxThread(%d)\n",
          _myInputQueue, _myOutputQueue, (int)_rxThread);
      if (ret) {
        if (mq_getattr(_myInputQueue, &_mymq_attr) < 0) {
          perror("CspadConfigurator::_startRxThread mq_getattr failed: ");
          ret = false;
        } else {
          int msgs = _mymq_attr.mq_curmsgs;
          if (msgs) {
            printf("CspadConfigurator::_startRxThread flushing %d messages\n", msgs);
            while (msgs--) {
              if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &priority) < 0) {
                perror("CspadConfigurator::_startRxThread  mq_receive failed");
                ret = false;
              }
            }
          }
          _myOut.type = Enable;
//          printf("CspadConfigurator::_startRxThread sending Enable to receive thread\n");
          if (mq_send(_myOutputQueue, (const char *)&_myOut, sizeof(_myOut), priority) < 0) {
            perror("CspadConfigurator::_startRxThreadmq_send Enable failed ");
            ret = false;
          } else {
            if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &priority) < 0) {
              perror("CspadConfigurator::_startRxThread mq_receive EnableAck failed ");
              ret = false;
            } else {
              if (_myIn.type != EnableAck) {
                printf("CspadConfigurator::_startRxThread did not get EnableAck");
                ret = false;
              } else {
//                printf("CspadConfigurator::_startRxThread got Enable acknowledgment\n");
              }
            }
          }
        }
      }
      return ret;
    }

    bool CspadConfigurator::_stopRxThread() {
      bool ret = true;
      Msg _myIn, _myOut;
      unsigned      priority = 0;
      struct mq_attr _mymq_attr;
      _mymq_attr.mq_maxmsg = 10L;
      _mymq_attr.mq_msgsize = (long int)sizeof(_myIn);
      _mymq_attr.mq_flags = 0L;
      _myOut.type = Disable;
//      printf("\nCspadConfigurator::_stopRxThread sending Disable to receive thread\n");
      usleep(500);
      if (mq_send(_myOutputQueue, (const char *)&_myOut, sizeof(_myOut), 0) < 0) {
        perror("CspadConfigurator::_startRxThreadmq_send Disable failed ");
        ret = false;
      } else {
        if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &priority) < 0) {
          perror("CspadConfigurator::_startRxThread mq_receive DisableAck failed ");
          ret = false;
        } else {
          if (_myIn.type != DisableAck) {
            printf("CspadConfigurator::_stopRxThread did not get DisableAck");
            ret = false;
          } else {
//            printf("CspadConfigurator::_stopRxThread got Disable acknowledgment\n");
            if (mq_getattr(_myInputQueue, &_mymq_attr) < 0) {
              perror("CspadConfigurator::_startRxThread mq_getattr failed: ");
              ret = false;
            } else {
              int msgs = _mymq_attr.mq_curmsgs;
              if (msgs) {
                printf("CspadConfigurator::_stopRxThread flushing %d messages\n", msgs);
                while (msgs--) {
                  if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &priority) < 0) {
                    perror("CspadConfigurator::_stopRxThread  mq_receive failed");
                    ret = false;
                  }
                }
              }
            }
          }
        }
      }
      return ret;
    }

    unsigned CspadConfigurator::configure(unsigned mask) {
      timespec      start, end, sleepTime;
      sleepTime.tv_sec = 0;
      sleepTime.tv_nsec = 25000000; // 25ms
      bool printFlag = !(mask & 0x2000);
      if (printFlag) printf("CXI Config");
      printf(" config(%p) quadMask(0x%x) mask(0x%x)\n", &_config, (unsigned)_config->quadMask(), ~mask);
      unsigned ret = 0;
      mask = ~mask;
      //    _print = true;
      clock_gettime(CLOCK_REALTIME, &start);
      if (_startRxThread()) {
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, Pds::CsPad::NoRunning);
        //      ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetAddr, 1);
        nanosleep(&sleepTime, 0);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetQuadsAddr, 1);
        nanosleep(&sleepTime, 0);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, Pds::CsPad::NoRunning);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, TriggerWidthAddr, TriggerWidthValue);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, ResetSeqCountRegisterAddr, 1);
        _print = false;
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
        if (mask&16 && ret==0) {
          if (printFlag) printf("- 0x%x - \n\treading ", ret);
          ret |= readRegs();
          if (printFlag) {
            clock_gettime(CLOCK_REALTIME, &end);
            uint64_t diff = timeDiff(&end, &start) + 50000LL;
            printf("\tso far %lld.%lld milliseconds\t", diff/1000000LL, diff%1000000LL);
          }
        }
        //    _print = true;
        ret <<= 1;
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, EnableEvrAddr, EnableEvrValue);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, resetCountersAddr, 1);
        ret |= writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, RunModeAddr, _config->inactiveRunMode());
        microSpin(25000);
        if (_stopRxThread() == false) {
          printf("CspadConfigurator::configure failed to stop the RX thread\n");
          ret <<= 1;
          ret |= 1;
        }
      } else {
        printf("CspadConfigurator::configure receive thread startup failed\n");
        ret = 1;
      }
      _print = false;
      if (printFlag) {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = timeDiff(&end, &start) + 50000LL;
        printf("- 0x%x - \n\tdone \n", ret);
        printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
        //      printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
        //    print();
      }
      return ret;
    }

    void CspadConfigurator::dumpFrontEnd() {
      timespec      start, end;
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned ret = Success;
      if (_debug & 0x100) {
        printf("CspadConfigurator::dumpFrontEnd: Concentrator\n");
        ret = _conRegs->read(_drdr);
        if (ret == Success) {
          _conRegs->print();
        } else {
          printf("\tCould not be read!\n");
        }
      }
      if (_debug & 0x200) {
        for (unsigned i=0; i<MaxQuadsPerSensor && ret == Success; i++) {
          unsigned ret2 = 0;
          if ((1<<i) & _config->quadMask()) {
            printf("CspadConfigurator::dumpFrontEnd: Quad %u\n", i);
            ret2 = _quadRegs->read(i, _drdr);
            if (ret2 == Success) {
              _quadRegs->print();
            } else {
              printf("\tCould not be read!\n");
            }
            ret |= ret2;
          }
        }
      }
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      if (_debug & 0x300) {
        printf("CspadConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
        printf(" - %s\n", ret == Success ? "Success" : "Failed!");
      }
      return;
    }

    unsigned CspadConfigurator::writeRegister(
        Pds::Pgp::RegisterSlaveExportFrame::FEdest dst,
        unsigned addr,
        uint32_t data,
        Pds::Pgp::RegisterSlaveExportFrame::waitState w) {
      Pds::Pgp::RegisterSlaveExportFrame rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::RegisterSlaveExportFrame::write,
              dst,
              addr,
              0x6969,
              data,
              w);
      if (_print) rsef.print();
      return rsef.post(sizeof(rsef)/sizeof(uint32_t));
    }

    unsigned CspadConfigurator::writeRegister(
        Pds::Pgp::RegisterSlaveExportFrame::FEdest dst,
        unsigned addr,
        uint32_t data,
        bool printFlag) {
      _print = printFlag;
      unsigned ret = writeRegister(dst, addr, data, Pds::Pgp::RegisterSlaveExportFrame::notWaiting);
      _print = false;
      return ret;
    }

    Pds::Pgp::RegisterSlaveImportFrame* CspadConfigurator::_readPgpCard() {
      Pds::Pgp::RegisterSlaveImportFrame* ret = 0;
      fd_set          fds;
      int             sret = 0;
      Msg             _myIn;
      unsigned        _priority = 0;
      struct timeval  timeout;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 200000;
      FD_ZERO(&fds);
      FD_SET(_myInputQueue,&fds);
      if ( (sret = select( _myInputQueue+1, &fds, NULL, NULL, &timeout)) > 0) {
        if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &_priority) < 0) {
          perror("CspadConfigurator::_readPgpCard mq_receive failed");
        } else {
          ret = _myIn.rsif;
        }
      } else {
        if (sret < 0) perror("CspadConfigurator::_readPgpCard select error ");
        else printf("CspadConfigurator::_readPgpCard select timed out\n");
      }
      if (ret != 0) {
        bool hardwareFailure = false;
        if (ret->failed()) {
          printf("CspadConfigurator::_readPgpCard receive HW failed\n");
          ret->print();
          hardwareFailure = true;
        }
        if (ret->timeout()) {
          printf("CspadConfigurator::_readPgpCard receive HW timed out\n");
          ret->print();
          hardwareFailure = true;
        }
        if (hardwareFailure) ret = 0;
//        else {
//          printf("CspadConfigurator::_readPgpCard returning input frame\n");
//        }
      }
      return ret;
    }

    unsigned CspadConfigurator::readRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest dest, unsigned addr, unsigned tid, uint32_t* retp) {
      Pds::Pgp::RegisterSlaveImportFrame* rsif;
      Pds::Pgp::RegisterSlaveExportFrame  rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
          Pds::Pgp::RegisterSlaveExportFrame::read,
          dest,
          addr,
          tid,
          (uint32_t)0,
          Pds::Pgp::RegisterSlaveExportFrame::Waiting);
      if (rsef.post(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) != Success) {
        printf("CspadConfigurator::readRegister failed, export frame follows.\n");
        rsef.print(0, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
        return  Failure;
      }
      unsigned errorCount = 0;
      while (true) {
        rsif = _readPgpCard();
        if (rsif == 0) {
          printf("CspadConfigurator::readRegister _readPgpCard failed!\n");
          return Failure;
        }
        if (addr != rsif->addr()) {
          printf("\nReadConfig out of order response dest=%u, addr=0x%x, tid=%u, errorCount=%u\n",
              dest, addr, tid, ++errorCount);
          rsif->print();
          if (errorCount > 5) return Failure;
        } else {
          *retp = rsif->data();
        } return Success;
      }
    }

    unsigned CspadConfigurator::readRegs() {
      if (Failure == readRegister(
          Pds::Pgp::RegisterSlaveExportFrame::CR,
          ConcentratorVersionAddr,
          0x55000,
          _config->concentratorVersionAddr())) {
        return Failure;
      }
      if (_debug & 0x10) printf("\nCspadConfigurator concentrator version 0x%x\n", *_config->concentratorVersionAddr());
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          uint32_t* u = (uint32_t*) _config->quads()[i].readOnly();
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          for (unsigned j=0; j<sizeOfQuadReadOnly; j++) {
            if (Failure == readRegister(dest, _quadReadOnlyAddrs[j], 0x55000 | (i<<4) | j, u+j)) {
              return Failure;
            }
          }
          if (_debug & 0x10) printf("CspadConfigurator Quad %u read only 0x%x 0x%x %p %p\n",
              i, (unsigned)u[0], (unsigned)u[1], u, _config->quads()[i].readOnly());
        }
      }
      return Success;
    }

    unsigned CspadConfigurator::writeRegs() {
      uint32_t* u;
      if (writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, EventCodeAddr, _config->eventCode())) {
        return Failure;
      }
      //    printf(" ev code %u", (unsigned)_config->eventCode());
      if (writeRegister(Pds::Pgp::RegisterSlaveExportFrame::CR, runDelayAddr, _config->runDelay())) {
        return Failure;
      }
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          u = (uint32_t*)&(_config->quads()[i]);
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
      Pds::Pgp::RegisterSlaveImportFrame* rsif;
      int             ret = Success;

      rsif = cfgrt->_readPgpCard();
      if (rsif == 0) {
        printf("CspadConfigSynch::_getOne _pgpCardRead failed\n");
        ret = Failure;
      }
      return ret;
    }

    bool CspadConfigSynch::take() {
      if (_depth == 0) {
        if (_getOne() == Failure) {
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
        if (_getOne() == Failure) {
          printf("CspadConfigSynch::clear receive clearing failed, missing %d\n", cnt);
          return false;
        }
        cnt--;
      }
      return true;
    }

    unsigned CspadConfigurator::writeDigPots() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      unsigned ret = Success;
      // in bytes, size of pots array minus the two in the header plus the NotSupposedToCare, written as uint32_t.
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + PotsPerQuad -2 + 1];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
      }
      CspadConfigSynch mySynch(_fd, 0, _rhisto, this);
      for (unsigned i=0; i<MaxQuadsPerSensor && ret == Success; i++) {
        if ((1<<i) & _config->quadMask()) {
          quadCount += 1;
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          //          printf("\n/tpot write, %u, 0x%x, %u", j, (unsigned)(_digPot.base + j), (unsigned)_config->quads()[i].dp().value(j));
          Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::RegisterSlaveExportFrame::write,
              dest,
              _digPot.base,
              i);
          uint32_t* bulk = rsef->array();
          for (unsigned j=0; j<PotsPerQuad; j++) {
            bulk[j] = (uint32_t)_config->quads()[i].dp().value(j);
          }
          bulk[PotsPerQuad] = 0;
          if (rsef->post(sizeof(myArray)/sizeof(uint32_t)) != Success) {
            printf("Writing pots failed\n");
            ret = Failure;
          }
          if (writeRegister(dest,
              _digPot.load,
              1,
              quadCount == numberOfQuads ? Pds::Pgp::RegisterSlaveExportFrame::Waiting
                  : Pds::Pgp::RegisterSlaveExportFrame::notWaiting)) {
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
      return ret;
    }

    unsigned CspadConfigurator::writeTestData() {
      unsigned ret = Success;
      unsigned numberOfQuads = 0;
      unsigned quadCount = 0;
      unsigned row = 0;
      // number of columns minus the two in the header and plus one for the dnc at the end
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) +  ColumnsPerASIC - 2 + 1];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
      }
      CspadConfigSynch mySynch(_fd, 0, _rhisto, this);
      Pds::Pgp::RegisterSlaveExportFrame* rsef = 0;
      for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) {
          quadCount += 1;
          Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
          for (row=0; row<RowsPerBank; row++) {
            rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
                Pds::Pgp::RegisterSlaveExportFrame::write,
                dest,
                quadTestDataAddr + (row<<8),  // hardware wants rows spaced 8 bits apart
                (i << 8) | row,
                0,
                (row == RowsPerBank-1) && (quadCount == numberOfQuads) ?
                    Pds::Pgp::RegisterSlaveExportFrame::Waiting :
                    Pds::Pgp::RegisterSlaveExportFrame::notWaiting);
            uint32_t* bulk = rsef->array();
            for (unsigned col=0; col<ColumnsPerASIC; col++) {
              bulk[col] = (uint32_t)rawTestData[_config->tdi()][row][col];
            }
            bulk[ColumnsPerASIC] = 0;
//            if (row == RowsPerBank-1) rsef->print(i, sizeof(myArray)/sizeof(uint32_t));
            rsef->post(sizeof(myArray)/sizeof(uint32_t));
            usleep(MicroSecondsSleepTime);
          }
          //        printf(";");
        }// else printf("skipping quad %u\n", i);
      }
      if (!mySynch.take()) {
        printf("Write Test Data synchronization failed! \n");
        rsef->print(row, sizeof(myArray)/sizeof(uint32_t));
        ret = Failure;
      }
      return ret;
    }

    enum {LoopHistoShift=8};

    void CspadConfigurator::print() {}

    unsigned CspadConfigurator::writeGainMap() {
      //    printf(" quadMask(0x%x) ", (unsigned)_config->quadMask());
      unsigned numberOfQuads = 0;
      Pds::CsPad::CsPadGainMapCfg::GainMap* map[Pds::CsPad::MaxQuadsPerSensor];
      // the first two are in the header, but we need one DNC for Ryan's logic
      //    one plus the address of the last word minus the two in the header plus the dnc.
      unsigned len = (((RowsPerBank-1)<<3) + BanksPerASIC-2 + 1) - 2 + 1;  // uint32_t's in the payload
      uint32_t myArray[(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) + len];
      for (int i=0; i<Pds::CsPad::MaxQuadsPerSensor; i++) {
        if ((1<<i) & _config->quadMask()) numberOfQuads += 1;
        map[i] = (Pds::CsPad::CsPadGainMapCfg::GainMap*)_config->quads()[i].gm()->map();
      }
      CspadConfigSynch mySynch(_fd, numberOfQuads, _rhisto, this);
      for (unsigned col=0; col<ColumnsPerASIC; col++) {
        for (unsigned i=0; i<MaxQuadsPerSensor; i++) {
          if ((1<<i) & _config->quadMask()) {
            Pds::Pgp::RegisterSlaveExportFrame::FEdest dest = (Pds::Pgp::RegisterSlaveExportFrame::FEdest)i;
            if (!mySynch.take()) {
              printf("Gain Map Write synchronization failed! col(%u), quad(%u)\n", col, i);
              return Failure;
            }
            Pds::Pgp::RegisterSlaveExportFrame* rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
                Pds::Pgp::RegisterSlaveExportFrame::write,
                dest,
                _gainMap.base,
                (col << 4) | i,
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
            rsef->post(sizeof(myArray)/sizeof(uint32_t));
            microSpin(MicroSecondsSleepTime);

            if(writeRegister(dest, _gainMap.load, col, Pds::Pgp::RegisterSlaveExportFrame::Waiting)) {
              return Failure;
            }
          }
        }
      }
      if (!mySynch.clear()) return Failure;
      return Success;
    }

    void* rxMain(void* p) {
      enum {SizeOfRxPool=8, SleepTimeUSec=2500, BufferWords=2048};
      enum RX_STATES {Disabled, Enabled};
      CspadConfigurator* cfgrt = (CspadConfigurator*)p;
      enum RX_STATES  _state = Disabled;
      unsigned        _buffer[BufferWords];
      int             _fd = cfgrt->fd();
      Msg             _myOut, _myIn;
      unsigned        _priority = 0;
      Pds::Pgp::RegisterSlaveImportFrame rsif;
      Pds::Pgp::RegisterSlaveImportFrame rsifPool[SizeOfRxPool];
      unsigned        rxPoolIndex = 0;
      fd_set          fds;
      struct timeval  timeout;
      PgpCardRx       pgpCardRx;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = BufferWords;
      pgpCardRx.data    = (__u32*)_buffer;
      struct mq_attr _mymq_attr;
      _mymq_attr.mq_maxmsg = 10L;
      _mymq_attr.mq_msgsize = (long int)sizeof(_myOut);
      _mymq_attr.mq_flags = 0L;

      mqd_t _myInputQueue = mq_open("/MsgToCsPadReceiverQueue", OFLAGS, PERMS, &_mymq_attr);
      if (_myInputQueue < 0) perror("CspadConfigurator rxMain mq_open receiver input");
      mqd_t _myOutputQueue = mq_open("/MsgFromCsPadReceiverQueue", OFLAGS, PERMS, &_mymq_attr);
      if (_myOutputQueue < 0) perror("CspadConfigurator rxMain mq_open receiver output");

      bool errorState = false;
      printf("CspadConfigurator rxMain starting loop _fd(%d) _myInputQueue(%d) _myOutputQueue(%d)\n",
          _fd, (int)_myInputQueue, (int)_myOutputQueue);
      while (1) {
        if (mq_getattr(_myInputQueue, &_mymq_attr) < 0) {
          if (!errorState) perror("CspadConfigurator rxMain mq_getattr failed: ");
          errorState = true;
        } else {
          if (_mymq_attr.mq_curmsgs > 0) {
            if (_mymq_attr.mq_curmsgs > 1) printf("CspadConfigurator rxMain found %d messages waiting\n", (int)_mymq_attr.mq_curmsgs);
            if (mq_receive(_myInputQueue, (char*)&_myIn, sizeof(_myIn), &_priority) < 0) {
              if (!errorState) perror("CspadConfigurator rxMain mq_receive failed");
              errorState = true;
            } else {
              switch (_myIn.type) {
                case Enable :
                  _state = Enabled;
//                  printf("CspadConfigurator rxMain going to Enabled state\n");
                  _myOut.type = EnableAck;
                  if (mq_send(_myOutputQueue, (const char *)&_myOut, sizeof(_myOut), 0) < 0) {
                    if (!errorState) perror("CspadConfigurator rxMain mq_send EnableAck");
                    errorState = true;
                  }
                  break;
                case Disable :
                  _state = Disabled;
//                  printf("CspadConfigurator rxMain going to Disabled state\n");
                  _myOut.type = DisableAck;
                  if (mq_send(_myOutputQueue, (const char *)&_myOut, sizeof(_myOut), 0) < 0) {
                    if (!errorState) perror("CspadConfigurator rxMain mq_send DisableAck");
                    errorState = true;
                  }
                  break;
                default:
                  if (!errorState) printf("CspadConfigurator rxMain unknown Message Type %d\n", _myIn.type);
                  errorState = true;
                  break;
              }
            }
          }
          if (errorState) {
            if (_state == Enabled) printf("CspadConfigurator rxMain going to disabled state due to error\n");
            _state = Disabled;
          }
          if (_state == Enabled) {
            int sret = 0;
            unsigned readRet = 0;
            timeout.tv_sec  = 0;
            timeout.tv_usec = SleepTimeUSec;
            FD_ZERO(&fds);
            FD_SET(_fd,&fds);
            if ((sret = select(_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
              readRet = ::read(_fd, &pgpCardRx, sizeof(PgpCardRx));
//              printf("CspadConfigurator rxMain read return %u ", readRet);
              if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
                printf("Cspad Config Receiver pgpcard error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
                    pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
                printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
              } else {
                memcpy(&rsif, _buffer, sizeof(rsif));
                if (rsif.failed()) {
                  printf("Cspad Config Receiver receive HW failed\n");
                  printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
                  rsif.print();
                }
                if (rsif.timeout()) {
                  printf("Cspad Config Receiver receive HW timed out\n");
                  printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
                  rsif.print();
                }
                if ((rsif.bits.waiting == Pds::Pgp::RegisterSlaveExportFrame::Waiting) ||
                    (rsif.bits.oc == Pds::Pgp::RegisterSlaveExportFrame::read)) {
                  ::memcpy( rsifPool + rxPoolIndex, &rsif, sizeof(rsif));
                  _myOut.type = PGPrsif;
                  _myOut.rsif = rsifPool + rxPoolIndex;
                  if (mq_send(_myOutputQueue, (const char *)&_myOut, sizeof(_myOut), 0) < 0) {
                    if (!errorState) perror("CspadConfigurator rxMain mq_send PGPrsif");
                    errorState = true;
                  }
                  rxPoolIndex += 1;
                  rxPoolIndex %= SizeOfRxPool;
//                  printf("sent to output queue\n");
                }
              }
            } else {
              if (sret < 0) {
                perror("Cspad Config Receiver receive select error: ");
                printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
              }
            }
          } else {
            usleep(SleepTimeUSec);
          }
        }
      }
      return 0;
    }  // rxMain
  } // namespace CsPad
} // namespace Pds

