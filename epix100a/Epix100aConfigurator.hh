/*
 * Epix100aConfigurator.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIX100A_CONFIGURATOR_HH_
#define EPIX100A_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/epix100a/Epix100aStatusRegisters.hh"
#include "pds/epix100a/Epix100aDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds {
  namespace Epix100a {
    //    AcqCount : 0x000005
    //    Can be reset by writing to 0x000006
    //
    //    SeqCount : 0x00000B
    //    Can be reset by writing to 0x00000C
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
      PowerEnableValue                  = 0x7,
      EnviroDataBaseAddr				= 0x140,
      RowCounterAdder                   = 0x6011,
      ColCounterAddr                    = 0x6013,
      PixelDataAddr                     = 0x5000,
      WriteColDataAddr                  = 0x3000,
      PrepareMultiConfigAddr            = 0x8000,
      MultiplePixelWriteCommandAddr     = 0x080000,
      TotalPixelsAddr                   = 0x27,
      PixelsPerBank                     = 96,
      BanksPerAsic                      = 4,
      SaciClkBitAddr                    = 0x28,
      SaciClkBitValue                   = 0x4,
      TopCalibRow                       = 0x10000, //0x00010000
      BottomCalibRow                    = 0x30000,
//      EnableAutomaticRunTriggerAddr     = 0x11,  // added to config
//      NumberClockTicksPerRunTriggerAddr = 0x12,
//      NumberClockTicksPerRunTrigger     = 0xFE503, // 120 Hz  add to config
//                                        // 0x1fca05 // 60 Hz
//                                        // 0x3f940b // 30 Hz
//                                        // 0xbebc20 // 10 Hz
//                                        //0x17d7840 //  5 Hz
//                                        //0x7735940 //  1 Hz
      EnableAutomaticDaqTriggerAddr     = 0x13,  // do not add to config
      DaqTrigggerDelayAddr              = 0x4,
      RunToDaqTriggerDelay              = 1250
    };

    enum asicControlAddrs {
      AsicAddrBase      = 0x800000,
      AsicAddrOffset    = 0x100000,
      WritePixelCommand =   0x8000,
      GlobalPixelCommand =   0x4000,
      PrepareForReadout =        0,
      RepeatControlCount = 1
    };

    enum useModes { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };

    enum enables { Disable=0, Enable=1 };

    class Epix100aConfigurator : public Pds::Pgp::Configurator {
      public:
        Epix100aConfigurator(int, unsigned);
        virtual ~Epix100aConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(Epix100aConfigType*, unsigned first=0);
        Epix100aConfigType&      configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        void                 runTimeConfigName(char*);
        uint32_t             testModeState() { return _testModeState; };
        void                 resetFrontEnd();
        void                 resetSequenceCount();
        uint32_t             sequenceCount();
        uint32_t             acquisitionCount();
        void                 enableExternalTrigger(bool);
        void                 enableRunTrigger(bool);
        void                 maintainLostRunTrigger(bool b) { _maintainLostRunTrigger = b; }
        uint32_t             enviroData(unsigned);


      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        unsigned             writeASIC();
        unsigned             writePixelBits();
        unsigned             checkWrittenASIC(bool writeBack=true);
        bool                 _flush(unsigned index=0);
        bool                 _getAnAnswer(unsigned size = 4, unsigned count = 6);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        uint32_t                    _testModeState;
        Epix100aConfigType*             _config;
        Epix100aConfigShadow*           _s;
        Epix100aDestination             _d;
        unsigned*                   _rhisto;
        char                       _runTimeConfigFileName[256];
//      LoopHisto*                _lhisto;
        bool                        _maintainLostRunTrigger;
    };

  }
}

#endif /* EPIX100A_CONFIGURATOR_HH_ */
