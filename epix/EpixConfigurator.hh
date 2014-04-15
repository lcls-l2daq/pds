/*
 * EpixConfigurator.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIX_CONFIGURATOR_HH_
#define EPIX_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/epix/EpixStatusRegisters.hh"
#include "pds/epix/EpixDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds {
  namespace Epix {
//    0x000011, bit 0 - enable automatic run triggers (only works if the normal RunTrigEnable at register 0x000001 is also set)
//    0x000012, bits 31:0 - number of clock cycles between autotriggers (for 120 Hz, this should be set to 1041667, or 0xFE503)
//    0x000013, bit 0 - enable automatic daq triggers (only works if the normal DaqTrigEnable at register 0x000003 is also set)


    enum controlAddrs {
      VersionAddr                       = 0x0,
      ResetAddr                         = 0x0,
      DaqTriggerEnable                  = 0x3,
      RunTriggerEnable                  = 0x1,
      ResetFrameCounter                 = 0xC,
      ReadFrameCounter                  = 0xB,
      ResetAcqCounter                   = 0x6,
      ReadAcqCounter                    = 0x5,
      PowerEnableAddr                   = 0x8,
      PowerEnableValue                  = 0x3,
      TotalPixelsAddr                   = 0x27,
      PixelsPerBank                     = 96,
      SaciClkBitAddr                    = 0x28,
      SaciClkBitValue                   = 0x4,
      EnableAutomaticRunTriggerAddr     = 0x11,  // add to config
      NumberClockTicksPerRunTriggerAddr = 0x12,
      NumberClockTicksPerRunTrigger     = 0xFE503, // 120 Hz  add to config
                                        // 0x1fca05 // 60 Hz
                                        // 0x3f940b // 30 Hz
                                        // 0xbebc20 // 10 Hz
                                        //0x17d7840 //  5 Hz
                                        //0x7735940 //  1 Hz
      EnableAutomaticDaqTriggerAddr     = 0x13,  // do not add to config
      DaqTrigggerDelayAddr              = 0x4,
      RunToDaqTriggerDelay              = 1250
    };

    enum asicControlAddrs {
      AsicAddrBase      = 0x800000,
      AsicAddrOffset    = 0x100000,
      WritePixelCommand =   0x8000,
      GloalPixelCommand =   0x4000,
      RepeatControlCount = 2
    };

    enum useModes { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };

    enum enables { Disable=0, Enable=1 };

    class EpixConfigurator : public Pds::Pgp::Configurator {
      public:
        EpixConfigurator(int, unsigned);
        virtual ~EpixConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(EpixConfigType*, unsigned first=0);
        EpixConfigType&      configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        void                 runTimeConfigName(char*);
        uint32_t             testModeState() { return _testModeState; };
        void                 resetFrontEnd();
        void                 resetSequenceCount();
        uint32_t             sequenceCount();
        void                 enableExternalTrigger(bool);
        void                 enableRunTrigger(bool);


      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        unsigned             writeASIC();
        unsigned             checkWrittenASIC(bool writeBack=true);
        unsigned             writePixelBits();
        bool                 _flush(unsigned index=0);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        uint32_t                    _testModeState;
        EpixConfigType*             _config;
        EpixConfigShadow*           _s;
        EpixDestination             _d;
        unsigned*                   _rhisto;
        char                       _runTimeConfigFileName[256];
//      LoopHisto*                _lhisto;
    };

  }
}

#endif /* EPIX_CONFIGURATOR_HH_ */
