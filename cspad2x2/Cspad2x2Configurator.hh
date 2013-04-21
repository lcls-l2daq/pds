/*
 * Cspad2x2Configurator.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPADCONFIGURATOR_HH_
#define CSPADCONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/cspad2x2/Cspad2x2QuadRegisters.hh"
#include "pds/cspad2x2/Cspad2x2ConcentratorRegisters.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds { namespace Pgp { class RegisterSlaveImportFrame; } }

namespace Pds {

  namespace CsPad2x2 {

    class Cspad2x2Configurator;
    class Cspad2x2ConcentratorRegisters;

    class Cspad2x2Configurator : public Pds::Pgp::Configurator {
      public:
        Cspad2x2Configurator( CsPad2x2ConfigType*, int, unsigned);
        virtual ~Cspad2x2Configurator();

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
            RunModeAddr=0x2a,
            ProtEnableAddr=0x3e,
            protThreshBase=0x36,
            internalTriggerDelayAddr=0x3f,
            externalTriggerDelayAddr=0x40,
            twoHunderedFiftyMicrosecondsIn8nsIncrements=31250
        };
        enum {externalTriggerDelayValue=0};
        enum {resetAddr=1,
            resetCountersAddr=2,
            resetLinksBaseAddr=3,
            resetSeqCountAddr=9,
            resetQuadsAddr=0x2C
        };
        enum potsAddrs {CompBias1addr=0x13, CompBias2addr=0x27, iss2addr=0x3b, iss5addr=0x4b};
        enum {TriggerWidthAddr=0x102, TriggerWidthValue=15};
        enum {ResetSeqCountRegisterAddr=0x000009};
        enum {QuadResetRegisterAddr=0x500004};

        unsigned                  configure(CsPad2x2ConfigType*, unsigned mask=0);
        CsPad2x2ConfigType&       configuration() { return *_config; };
        void                      print();
        void                      dumpFrontEnd();
        void                      printMe();
        void                      runTimeConfigName(char*);

        static unsigned           _quadAddrs[];
        static unsigned           _quadReadOnlyAddrs[];
        static uint16_t           rawTestData[][Pds::CsPad2x2::RowsPerBank][Pds::CsPad2x2::ColumnsPerASIC];

      private:

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
        resultReturn              _checkReg(Cspad2x2Destination*, unsigned, unsigned, uint32_t);
        unsigned                  _internalColWrite(uint32_t, bool, bool,
                                                   Pds::Pgp::RegisterSlaveExportFrame*, unsigned);
        unsigned                  _internalColWrite(uint32_t, bool,
                                                   Pds::Pgp::RegisterSlaveExportFrame*, uint32_t);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {quadGainMapStartAddr=0, quadGainMapLoadAddr=0x10000, quadTestDataAddr=0x100000};
        enum {MicroSecondsSleepTime=50};
        CsPad2x2ConfigType*            _config;
        Cspad2x2Destination            _d;
        Pds::Pgp::AddressRange         _gainMap;
        Pds::Pgp::AddressRange         _digPot;
        unsigned*                      _rhisto;
        Cspad2x2ConcentratorRegisters* _conRegs;
        Cspad2x2QuadRegisters*         _quadRegs;
        char                           _runTimeConfigFileName[256];
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* CSPADCONFIGURATOR_HH_ */
