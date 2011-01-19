/*
 * CspadConfigurator.hh
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#ifndef CSPADCONFIGURATOR_HH_
#define CSPADCONFIGURATOR_HH_

#include "pds/config/CsPadConfigType.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <new>
#include <mqueue.h>

namespace Pds { namespace CsPad { class ConfigV1QuadReg; } }

namespace Pds { namespace Pgp { class RegisterSlaveImportFrame; } }

namespace Pds {

  namespace CsPad {

    class CspadConfigurator;

    class AddressRange {
      public:
        AddressRange() {};
        AddressRange(uint32_t b, uint32_t l) : base(b), load(l) {};
        ~AddressRange() {};

        uint32_t base;
        uint32_t load;
    };

    class CspadConfigSynch {
      public:
        CspadConfigSynch(int fd, uint32_t d, unsigned* h,CspadConfigurator* c) :
          _depth(d), _length(d), _histo(h), _fd(fd), cfgrt(c) {};
        bool take();
        bool clear();

      private:
        enum {Success=0, Failure=1};
        unsigned      _getOne();
        unsigned _depth;
        unsigned _length;
        unsigned* _histo;
        int       _fd;
        CspadConfigurator* cfgrt;
    };

    class CspadConfigurator {
      public:
        CspadConfigurator( CsPadConfigType*, int, unsigned);
        virtual ~CspadConfigurator();

        enum {Success=0, Failure=1, numberTestImages=8};
        enum {ConcentratorVersionAddr=0,
            EnableEvrAddr=0x100,
            EnableEvrValue=1,
            EventCodeAddr=0x146,
            RunModeAddr=0x2a};
        enum {resetAddr=1,
            resetCountersAddr=2,
            resetLinksBaseAddr=3,
            resetSeqCountAddr=9,
            resetQuadsAddr=0x2C
        };
        enum {TriggerWidthAddr=0x102, TriggerWidthValue=15};
        enum {ResetSeqCountRegisterAddr=0x000009};
        enum {QuadResetRegisterAddr=0x500004};

        unsigned                  configure(unsigned mask=0);
        CsPadConfigType&          configuration() { return *_config; };
        void                      print();
        unsigned                  writeRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, uint32_t, bool pf=false);
        unsigned                  writeRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, uint32_t, Pds::Pgp::RegisterSlaveExportFrame::waitState);
        unsigned                  readRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, unsigned, uint32_t*);
        void                      printMe();
        int                       fd() { return _fd; }

        static unsigned           _quadAddrs[];
        static unsigned           _quadReadOnlyAddrs[];
        static uint16_t           rawTestData[][Pds::CsPad::RowsPerBank][Pds::CsPad::ColumnsPerASIC];

      private:
        static char                       _inputQueueName[80];
        static char                       _outputQueueName[80];

      private:
        unsigned                   writeRegs();
        unsigned                   writeDigPots();
        unsigned                   writeTestData();
        unsigned                   writeGainMap();
        unsigned                   readRegs();
        long long int              timeDiff(timespec*, timespec*);
        bool                      _startRxThread();
        bool                      _stopRxThread();
        Pds::Pgp::RegisterSlaveImportFrame*   _readPgpCard();
        void                      _initRanges() {
          new ((void*)&_gainMap) AddressRange(0x000000, 0x010000);
          new ((void*)&_digPot)  AddressRange(0x200000, 0x210000);
        }

      private:
        friend class CspadConfigSynch;
        typedef unsigned     LoopHisto[4][10000];
        enum {runDelayAddr=0x101};
        enum {sizeOfQuadWrite=18, sizeOfQuadReadOnly=2};
        enum {quadGainMapStartAddr=0, quadGainMapLoadAddr=0x10000, quadTestDataAddr=0x100000};
        enum {RtemsQueueTimeout=50, MicroSecondsSleepTime=50};
        CsPadConfigType*          _config;
        int                       _fd;
        AddressRange              _gainMap;
        AddressRange              _digPot;
        unsigned*                 _rhisto;
        pthread_t                 _rxThread;
        mqd_t                     _myInputQueue;
        mqd_t                     _myOutputQueue;
        unsigned                  _debug;
        //      LoopHisto*                _lhisto;
        bool                      _print;
    };

  }
}

#endif /* CSPADCONFIGURATOR_HH_ */
