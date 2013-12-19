/*
 * EpixConfigV1.hh
 *
 *  Created on: Nov 5, 2013
 *      Author: jackp
 */

#ifndef EPIXCONFIGV1_HH_
#define EPIXCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
//#include "pds/config/EpixASICConfigV1.hh"
#include <stdint.h>



namespace Pds {
  namespace EpixConfig {

    class EpixASICConfigV1;

    class ConfigV1 {
      public:
        ConfigV1(bool init=false);
        ~ConfigV1();
        enum Registers {
          Version,
          RunTrigDelay,
          DaqTrigDelay,
          DacSetting,
          asicGR,
          asicGRControl,
          asicAcq,
          asicAcqControl,
          asicR0,
          asicR0Control,
          asicPpmat,
          asicPpmatControl,
          asicPpbe,
          asicPpbeControl,
          asicRoClk,
          asicRoClkControl,
          prepulseR0En,
          adcStreamMode,
          testPatternEnable,
          AcqToAsicR0Delay,
          AsicR0ToAsicAcq,
          AsicAcqWidth,
          AsicAcqLToPPmatL,
          AsicRoClkHalfT,
          AdcReadsPerPixel,
          AdcClkHalfT,
          AsicR0Width,
          AdcPipelineDelay,
          PrepulseR0Width,
          PrepulseR0Delay,
          DigitalCardId0,
          DigitalCardId1,
          AnalogCardId0,
          AnalogCardId1,
          LastRowExclusions,
          NumberOfAsicsPerRow,
          NumberOfAsicsPerColumn,
          NumberOfRowsPerAsic,
          NumberOfPixelsPerAsicRow,
          BaseClockFrequency,
          AsicMask,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2 };

        enum {
          NumberOfValues=28
        };

        enum {
          NumberOfAsics=4    //  Kludge ALERT!!!!!  this needs to be dynamic !!
        };

        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        EpixASICConfigV1*           asics() { return (EpixASICConfigV1*) (this+1); }
        uint32_t                    numberOfAsics();
        static uint32_t             offset    (Registers);
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
//        uint32_t*                 pixelTestArray() { return _pixelTestArray; }
//        uint32_t*                 pixelMaskArray() { return _pixelMaskArray; }

      private:
        uint32_t     _values[NumberOfValues];
//        ASIC_ConfigV1 _asics[NumberOfAsics];
//        uint32_t     _pixelTestArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
//        uint32_t     _pixelMaskArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
    };

  } /* namespace ConfigV1 */
} /* namespace Pds */

#endif /* EPIXCONFIGV1_HH_ */
