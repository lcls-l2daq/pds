/*
 * CspadConfigurator.hh
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#ifndef CSPADCONFIGURATOR_HH_
#define CSPADCONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/cspad/CspadQuadRegisters.hh"
#include "pds/cspad/CspadConcentratorRegisters.hh"
#include "pds/cspad/CspadDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds { namespace Pgp { class RegisterSlaveImportFrame; } }

namespace Pds {

  namespace CsPad {

    class CspadConfigurator;
    class CspadConcentratorRegisters;
    class CspadQuadRegisters;

    class CspadConfigurator : public Pds::Pgp::Configurator {
      public:
        CspadConfigurator( CsPadConfigType*, int, unsigned);
        virtual ~CspadConfigurator();

        //   evrEnable.setAddr        (0x000100);
        //   evrOpCode.setAddr        (0x000146);
        //   evrDelay.setAddr         (0x000101);  (runDelay !!!!!!!)
        //   evrWidth.setAddr         (0x000102)
        //
        //            cxiCon->evrEnable.set(0x3);
        //            cxiCon->runMode.set(runMode->currentItem());
        //            cxiCon->evrDelay.set(evrDelay->value());
        //            cxiCon->evrWidth.set(0x15)

        enum resultReturn {Success=0, Failure=1, Terminate=2};
        enum {ConcentratorVersionAddr=0,
            EnableEvrAddr=0x100,
            EnableEvrValue=1,
            runDelayAddr=0x101,
            EvrWidthAddr=0x102,
            EvrWidthValue=0x15,
            EventCodeAddr=0x146,
            RunModeAddr=0x2a,
            ProtEnableAddr=0x3e,
            protThreshBase=0x36};
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
        void                      dumpFrontEnd();
        void                      printMe();

        static unsigned           _quadAddrs[];
        static unsigned           _quadReadOnlyAddrs[];
        static uint16_t           rawTestData[][Pds::CsPad::RowsPerBank][Pds::CsPad::ColumnsPerASIC];

      private:
//        static char                       _inputQueueName[80];
//        static char                       _outputQueueName[80];

      private:
        unsigned                   writeRegs();
        unsigned                   checkWrittenRegs();
        unsigned                   writeDigPots();
        unsigned                   checkDigPots();
        unsigned                   writeTestData();
        unsigned                   writeGainMap();
        unsigned                   readRegs();
        bool                      _flush(unsigned);
        void                      _initRanges();
        resultReturn              _checkReg(CspadDestination*, unsigned, unsigned, uint32_t);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {sizeOfQuadWrite=18, sizeOfQuadReadOnly=2};
        enum {quadGainMapStartAddr=0, quadGainMapLoadAddr=0x10000, quadTestDataAddr=0x100000};
        enum {RtemsQueueTimeout=50, MicroSecondsSleepTime=50};
        CsPadConfigType*            _config;
        CspadDestination                 _d;
        Pds::Pgp::AddressRange      _gainMap;
        Pds::Pgp::AddressRange      _digPot;
        unsigned*                   _rhisto;
        CspadConcentratorRegisters* _conRegs;
        CspadQuadRegisters*         _quadRegs;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* CSPADCONFIGURATOR_HH_ */
