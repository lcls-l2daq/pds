/*
 * FexampConfigurator.hh
 *
 *  Created on: Nov 6, 2011
 *      Author: jackp
 */

#ifndef FEXAMP_CONFIGURATOR_HH_
#define FEXAMP_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/FexampConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/fexamp/FexampInternalRegisters.hh"
#include "pds/fexamp/FexampDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds { namespace Pgp { class RegisterSlaveImportFrame; } }

namespace Pds {

  namespace Fexamp {

    class FexampConfigurator;
    class FexampInternalRegisters;

    class FexampConfigurator : public Pds::Pgp::Configurator {
      public:
        FexampConfigurator( FexampConfigType*, int, unsigned);
        virtual ~FexampConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        enum InternalConsts {
          RegisterAddr=0,
          resetAddr=0xa,
          MasterResetMask=1,
          RelinkMask=2,
          CountResetMask=4,
          LaneIdAddr=0x100,
          LaneIdStuffing=0xFEDCBA00
        };

        enum ExternalConsts {
          ReadoutTimingUpdateAddr=0xf,
          RunModeAddr = 0x10,
          RunModeValue = 8,
          ClearFrameCountAddr=0x12,
          ClearFrameCountValue=1,
          SwitcherControlAddr = 0x50,
          SwitcherControlValue = 0xb
        };

        enum ASIC_consts{
          ASIC_topBaseAddr = 0x8780,
          ASIC_addrOffset = 0x80,
          ASIC_triggerAddr = 0x100,
          ASIC_triggerValue = 1
        };

        unsigned             configure(unsigned mask=0);
        FexampConfigType&     configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        uint32_t             testModeState() { return _testModeState; };

      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        unsigned             writeASICs();
        unsigned             checkWrittenASICs(bool writeBack=true);
        bool                 _flush(unsigned);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        uint32_t                    _testModeState;
        FexampConfigType*            _config;
        FexampDestination                 _d;
        unsigned*                   _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* FEXAMP_CONFIGURATOR_HH_ */
