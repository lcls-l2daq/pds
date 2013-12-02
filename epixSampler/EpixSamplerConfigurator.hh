/*
 * EpixSamplerConfigurator.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXSAMPLER_CONFIGURATOR_HH_
#define EPIXSAMPLER_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/epixSampler/EpixSamplerDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>


namespace Pds {

  namespace EpixSampler {

// Configureation file provided by Kurtis ...
//    0x01 0x00000001  RunTriggerEnable
//    0x02 0x00000000  RunTrigDelay
//    0x03 0x00000001  DaqTriggerEnable
//    0x04 0x00000000  DaqTrigDelay
//    0x07 0x00000000  DaqSetting
//    0x08 0x00000003
//    0x09 0x00000000
//    0x29 0x0000000d
//    0x20 0x00000ea6
//    0x21 0x00000fa0
//    0x22 0x00000fa0
//    0x24 0x0000000d  AdcClkHalfT
//    0x23 0x000000fa
//    0x24 0x0000000d  AdcClkHalfT
//    0x27 0x00008400
//    0x25 0x00000001
//    0x26 0x00000002
//    0x28 0x00000005
//    0x2A 0x00000100  TestPatternEnable
//    0x2b 0x00000026
//    0x2c 0x00000011  AdcPipelineDelay
//    0x2d 0x00000000
//    0x2e 0x00000000
//    0x2f 0x00000000


    enum enableMasks { OFF=0, ON=1 };
//    enum controlAddrs {
//      ResetAddr         = 0x1000000,
//      DaqTriggerEnable  = 0x1000003,
//      RunTriggerEnable  = 0x1000001,
//      ResetFrameCounter = 0x100000C,
//      ReadFrameCounter  = 0x100000B,
//      ReadAcqCounter    = 0x1000006
//    };

    enum controlAddrs {
      ResetAddr         = 0x0,
      DaqTriggerEnable  = 0x3,
      RunTriggerEnable  = 0x1,
      ResetFrameCounter = 0xC,
      ReadFrameCounter  = 0xB,
      ResetAcqCounter   = 0x6,
      ReadAcqCounter    = 0x5
    };

    enum enables { Disable=0, Enable=1 };

    class EpixSamplerConfigurator : public Pds::Pgp::Configurator {
      public:
        EpixSamplerConfigurator(int, unsigned);
        virtual ~EpixSamplerConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(EpixSamplerConfigType*, unsigned mask=0);
        EpixSamplerConfigType&    configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        void                 resetFrontEnd(uint32_t r=0);
        void                 resetSequenceCount();
        uint32_t             sequenceCount();
        uint32_t             acquisitionCount();
        unsigned             firmwareVersion(unsigned*);
        void                 enableExternalTrigger(bool);


      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        bool                 _flush(unsigned);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        EpixSamplerConfigType*      _config;
        EpixSamplerConfigShadow*    _s;
        EpixSamplerDestination      _d;
        unsigned*                   _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* EPIXSAMPLER_CONFIGURATOR_HH_ */
