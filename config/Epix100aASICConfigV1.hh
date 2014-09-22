/*
 * Epix100aASICConfigV1.hh
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#ifndef EPIX100AASICCONFIGV1_HH_
#define EPIX100AASICCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include <stdint.h>
#include <string.h>

namespace Pds {
  namespace Epix100aConfig {

    class ASIC_ConfigV1 {
      public:
        ASIC_ConfigV1(bool init=false);
        ~ASIC_ConfigV1();

        enum Registers {
          RowStartAddr,
          RowStopAddr,
          ColumnStartAddr,
          ColumnStopAddr,
          chipID,
          automaticTestModeEnable,
          balconyDriverCurrent,
          balconyEnable,
          bandGapReferenceTemperatureCompensationBits,
          CCK_RegDelayEnable,
          digitalMonitor1Enable,
          digitalMonitor2Enable,
          digitalMonitorMux1,
          digitalMonitorMux2,
          dummyMask,
          dummyTest,
          EXEC_DelayEnable,
          extraRowsLowReferenceValue,
          fastPowerPulsingEnable,
          fastPowerPulsingSpeed,
          highResolutionModeTest,
          interleavedReadOutDisable,
          LVDS_ImpedenceMatchingEnable,
          outputDriverDacReferenceBias,
          outputDriverDrivingCapabilitiesAndStability,
          outputDriverInputCommonMode0,
          outputDriverInputCommonMode1,
          outputDriverInputCommonMode2,
          outputDriverInputCommonMode3,
          outputDriverOutputDynamicRange0,
          outputDriverOutputDynamicRange1,
          outputDriverOutputDynamicRange2,
          outputDriverOutputDynamicRange3,
          outputDriverTemperatureCompensationEnable,
          outputDriverTemperatureCompensationGain0,
          outputDriverTemperatureCompensationGain1,
          outputDriverTemperatureCompensationGain2,
          outputDriverTemperatureCompensationGain3,
          pixelBuffersAndPreamplifierDrivingCapabilities,
          pixelFilterLevel,
          pixelOutputBufferCurrent,
          preamplifierCurrent,
          programmableReadoutDelay,
          pulserCounterDirection,
          pulserReset,
          pulserSync,
          pulserVsPixelOnDelay,
          syncPinEnable,
          testBackEnd,
          testMode,
          testModeWithDarkFrame,
          testPointSystemInputCommonMode,
          testPointSystemOutputDynamicRange,
          testPointSystemTemperatureCompensationEnable,
          testPointSystemTemperatureCompensationGain,
          testPointSytemInputSelect,
          testPulserCurrent,
          testPulserLevel,
          VRefBaselineDac,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };
        enum copyStates { DoCopy=0, DoNotCopy=1 };

        enum {
          NumberOfValues=25
        };

        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        void                        clear    ();
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        static unsigned             doNotCopy(Registers);
        void                        operator=(ASIC_ConfigV1&);
        bool                        operator==(ASIC_ConfigV1&);
        bool                        operator!=(ASIC_ConfigV1& foo) { return !(*this==foo); }

      private:
        uint32_t              _values[NumberOfValues];
    };

  } /* namespace Epix100aConfig */
} /* namespace Pds */

#endif /* EPIX100AASICCONFIGV1_HH_ */
