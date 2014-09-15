/*
 * Epix10kConfigV1.hh
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#ifndef EPIX10KCONFIGV1_HH_
#define EPIX10KCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/Epix10kASICConfigV1.hh"
#include <stdint.h>



namespace Pds {
  namespace Epix10kConfig {

    class ASIC_ConfigV1;

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
          SyncMode,
          R0Mode,
          DoutPipeLineDelay,
          AcqToAsicR0Delay,
          AsicR0ToAsicAcq,
          AsicAcqWidth,
          AsicAcqLToPPmatL,
          AsicRoClkHalfT,
          AdcReadsPerPixel,
          AdcClkHalfT,
          AsicR0Width,
          AdcPipelineDelay,
          SyncWidth,
          SyncDelay,
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
          ScopeEnable,
          ScopeTrigEdge,
          ScopeTrigCh,
          ScopeArmMode,
          ScopeAdcThresh,
          ScopeHoldoff,
          ScopeOffset,
          ScopeTraceLength,
          ScopeSkipSamples,
          ScopeInputA,
          ScopeInputB,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2 };

        enum {
          NumberOfValues=34
        };

        enum {
          NumberOfAsics=4    //  Kludge ALERT!!!!!  this may need to be dynamic !!
        };

        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        void                        clear();
        ASIC_ConfigV1*              asics() { return (ASIC_ConfigV1*) (this+1); }
        const ASIC_ConfigV1*        asics() const { return (const ASIC_ConfigV1*) (this+1); }
        uint32_t                    numberOfAsics() const;
        static uint32_t             offset    (Registers);
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        static unsigned             address   (unsigned); // address of 32-bit access
        static unsigned             mask      (unsigned); // read mask of 32-bit access
        static readOnlyStates       use       (unsigned);
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

#endif /* EPIX10KCONFIGV1_HH_ */
