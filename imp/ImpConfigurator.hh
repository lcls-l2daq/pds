/*
 * ImpConfigurator.hh
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#ifndef IMP_CONFIGURATOR_HH_
#define IMP_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/imp/ImpStatusRegisters.hh"
#include "pds/imp/ImpDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

//Register definitions:
//25 = 0xDEAD0001 usRxReset,
//        0x00000002 is Count Reset,
//        0xDEAD0004 Reset_sft.
//
//26 = 0x00000001 enable data frames.
//
//27 = number of samples
//
//12 = (31:28) powers okay
//        (27:18) zeroes
//        (17) usRemLinked
//        (16) usLocLinked
//        (15:12) usRxCount
//        (11:8).  UsCellErrCount
//        (7:4).    UsLinkDownCount
//        (3:0).    UsLinkErrCount.
//
//24 = same as 12, except that the upper 4 bits are zeroes.
//

namespace Pds {

  namespace Imp {


    enum resetValues { usRxReset=0xDEAD0001, CountReset=2, SoftReset=0xDEAD0004 };
    enum runControlValues { EnableDataFramesValue=1 };

    class ImpConfigurator : public Pds::Pgp::Configurator {
      public:
        ImpConfigurator(int, unsigned);
        virtual ~ImpConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(ImpConfigType*, unsigned mask=0);
        ImpConfigType&    configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        uint32_t             testModeState() { return _testModeState; };
//        void                 resetFrontEnd(uint32_t);
        void                 resetSequenceCount();
        uint32_t             sequenceCount();
        void                 runTimeConfigName(char*);

      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        bool                 _flush(unsigned);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        uint32_t                 _testModeState;
        ImpConfigType*           _config;
        ImpDestination           _d;
        ImpStatusRegisters       _statRegs;
        uint32_t                 _runControl;
        char                     _runTimeConfigFileName[256];
        unsigned*                _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* IMP_CONFIGURATOR_HH_ */
