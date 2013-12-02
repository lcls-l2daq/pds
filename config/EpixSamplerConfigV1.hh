/*
 * EpixSamplerConfigV1.hh
 *
 *  Created on: Nov 5, 2013
 *      Author: jackp
 */

#ifndef EPIXCONFIGV1_HH_
#define EPIXCONFIGV1_HH_
#include "pdsdata/psddl/epixsampler.ddl.h"
#include <stdint.h>

namespace Pds {
  namespace EpixSamplerConfig {

   class ConfigV1 {
      public:
        ConfigV1(bool init=false);
        ~ConfigV1();
        enum Registers {
          Version,
          RunTrigDelay,
          DaqTrigDelay,
          DaqSetting,
          AdcClkHalfT,
          AdcPipelineDelay,
          DigitalCardId0,
          DigitalCardId1,
          AnalogCardId0,
          AnalogCardId1,
          NumberOfChannels,
          SamplesPerChannel,
          BaseCLockFrequency,
          TestPatternEnable,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2 };

        enum {
          NumberToWrite=DigitalCardId0, NumberOfValues=NumberOfRegisters
        };

        unsigned                    get      (Registers);
        void                        set      (Registers, unsigned);
        static char*                name     (Registers, bool init=false);
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);

      private:
        uint32_t     _values[NumberOfValues];
    };

  } /* namespace EpixSamplerConfig */
} /* namespace Pds */

#endif /* EPIXCONFIGV1_HH_ */
