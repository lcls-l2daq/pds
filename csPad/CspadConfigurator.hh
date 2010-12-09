/*
 * CspadConfigurator.hh
 *
 *  Created on: Apr 21, 2010
 *      Author: jackp
 */

#ifndef CSPADCONFIGURATOR_HH_
#define CSPADCONFIGURATOR_HH_

#include "pds/config/CsPadConfigType.hh"
//#include "rce/pic/Pool.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
//#include "rceusr/cxi/CxiPgpHandler.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds { namespace CsPad { class ConfigV1QuadReg; } }

namespace Pds {

  namespace CsPad {

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
        CspadConfigSynch(int fd, uint32_t d, unsigned* h) :
          _depth(d), _length(d), _histo(h) {};
        bool take();
        bool clear();

      private:
        enum {Success=0, Failure=1};
        unsigned      _getOne();
        unsigned _depth;
        unsigned _length;
        unsigned* _histo;
        int       _fd;
    };

    class CspadConfigurator {
      public:
        CspadConfigurator( CsPadConfigType&, int);
        virtual ~CspadConfigurator() {}

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
        void                      print();
        unsigned                  writeRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, uint32_t, bool pf=false);
        unsigned                  writeRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, uint32_t, Pds::Pgp::RegisterSlaveExportFrame::waitState);
        unsigned                  readRegister(Pds::Pgp::RegisterSlaveExportFrame::FEdest, unsigned, unsigned, uint32_t*);
        void                      printMe();

        static unsigned           _quadAddrs[];
        static unsigned           _quadReadOnlyAddrs[];
        static uint16_t           rawTestData[][Pds::CsPad::RowsPerBank][Pds::CsPad::ColumnsPerASIC];

      private:
        unsigned                   writeRegs();
        unsigned                   writeDigPots();
        unsigned                   writeTestData();
        unsigned                   writeGainMap();
        unsigned                   read();
        long long int              timeDiff(timespec*, timespec*);
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
        enum {RtemsQueueTimeout=50};
        CsPadConfigType&          _config;
        int                       _fd;
        AddressRange              _gainMap;
        AddressRange              _digPot;
        unsigned*                 _rhisto;
        //      LoopHisto*                _lhisto;
        bool                      _print;
    };

  }
}

#endif /* CSPADCONFIGURATOR_HH_ */
