/*
 * Epix100aASICConfigV1.cc
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#include <stdio.h>
#include "Epix100aASICConfigV1.hh"

namespace Pds {
  namespace Epix100aConfig {

    class Register {
      public:
        uint32_t offset;
        uint32_t shift;
        uint32_t mask;
        uint32_t defaultValue;
        ASIC_ConfigV1::readOnlyStates readOnly;
        ASIC_ConfigV1::copyStates doNotCopy;
    };

    static uint32_t _ASICfoo[][6] = {
   //offset  shift    mask   default readOnly doNotCopy
      { 16,    0,     0x1ff,     0,     3,    0}, //  RowStartAddr
      { 17,    0,     0x1ff,     97,    3,    0}, //  RowStopAddr
      { 18,    0,     0x7f,      0,     3,    0}, //  ColumnStartAddr
      { 19,    0,     0x7f,      95,    3,    0}, //  ColumnStopAddr
      { 20,    0,     0xffff,    0,     1,    0}, //  chipID
      { 2,     11,    0x1,       0,     0,    0}, //  automaticTestModeEnable
      { 9,     1,     0x7,       3,     0,    0}, //  balconyDriverCurrent
      { 9,     0,     0x1,       1,     0,    0}, //  balconyEnable
      { 12,    0,     0x3,       0,     0,    0}, //  bandGapReferenceTemperatureCompensationBits
      { 15,    3,     0x1,       0,     0,    0}, //  CCK_RegDelayEnable
      { 5,     0,     0x1,       0,     0,    0}, //  digitalMonitor1Enable
      { 5,     1,     0x1,       0,     0,    0}, //  digitalMonitor2Enable
      { 3,     0,     0xf,       0,     0,    0}, //  digitalMonitorMux1
      { 3,     4,     0xf,       1,     0,    0}, //  digitalMonitorMux2
      { 1,     1,     0x1,       0,     0,    0}, //  dummyMask
      { 1,     0,     0x1,       0,     0,    0}, //  dummyTest
      { 15,    2,     0x1,       0,     0,    0}, //  EXEC_DelayEnable
      { 6,     6,     0x3,       0,     0,    0}, //  extraRowsLowReferenceValue
      { 9,     7,     0x1,       1,     0,    0}, //  fastPowerPulsingEnable
      { 9,     4,     0x7,       3,     0,    0}, //  fastPowerPulsingSpeed
      { 2,     14,    0x1,       0,     0,    0}, //  highResolutionModeTest
      { 15,    1,     0x1,       0,     0,    0}, //  interleavedReadOutDisable
      { 5,     4,     0x1,       0,     0,    0}, //  LVDS_ImpedenceMatchingEnable
      { 12,    5,     0x7,       3,     0,    0}, //  outputDriverDacReferenceBias
      { 12,    2,     0x7,       3,     0,    0}, //  outputDriverDrivingCapabilitiesAndStability
      { 14,    2,     0x3f,      22,    0,    0}, //  outputDriverInputCommonMode0
      { 22,    2,     0x3f,      22,    0,    0}, //  outputDriverInputCommonMode1
      { 23,    2,     0x3f,      22,    0,    0}, //  outputDriverInputCommonMode2
      { 24,    2,     0x3f,      22,    0,    0}, //  outputDriverInputCommonMode3
      { 8,     0,     0xf,       3,     0,    0}, //  outputDriverOutputDynamicRange0
      { 8,     4,     0xf,       3,     0,    0}, //  outputDriverOutputDynamicRange1
      { 21,    0,     0xf,       3,     0,    0}, //  outputDriverOutputDynamicRange2
      { 21,    4,     0xf,       3,     0,    0}, //  outputDriverOutputDynamicRange3
      { 11,    0,     0x1,       1,     0,    0}, //  outputDriverTemperatureCompensationEnable
      { 14,    0,     0x3,       1,     0,    0}, //  outputDriverTemperatureCompensationGain0
      { 22,    0,     0x3,       1,     0,    0}, //  outputDriverTemperatureCompensationGain1
      { 23,    0,     0x3,       1,     0,    0}, //  outputDriverTemperatureCompensationGain2
      { 24,    0,     0x3,       1,     0,    0}, //  outputDriverTemperatureCompensationGain3
      { 10,    6,     0x3,       1,     0,    0}, //  pixelBuffersAndPreamplifierDrivingCapabilities
      { 11,    1,     0x3f,      33,    0,    0}, //  pixelFilterLevel
      { 10,    3,     0x7,       4,     0,    0}, //  pixelOutputBufferCurrent
      { 10,    0,     0x7,       4,     0,    0}, //  preamplifierCurrent
      { 7,     5,     0x7,       3,     0,    0}, //  programmableReadoutDelay
      { 2,     10,    0x1,       0,     0,    0}, //  pulserCounterDirection
      { 2,     15,    0x1,       0,     0,    0}, //  pulserReset
      { 0,     7,     0x1,       0,     0,    0}, //  pulserSync
      { 0,     0,     0x7,       0,     0,    0}, //  pulserVsPixelOnDelay
      { 15,    4,     0x1,       0,     0,    0}, //  syncPinEnable
      { 15,    0,     0x1,       0,     0,    0}, //  testBackEnd
      { 2,     12,    0x1,       0,     0,    0}, //  testMode
      { 2,     13,    0x1,       0,     0,    0}, //  testModeWithDarkFrame
      { 13,    2,     0x3f,      22,    0,    0}, //  testPointSystemInputCommonMode
      { 4,     4,     0x7,       3,     0,    0}, //  testPointSystemOutputDynamicRange
      { 7,     0,     0x1,       1,     0,    0}, //  testPointSystemTemperatureCompensationEnable
      { 13,    0,     0x3,       0,     0,    0}, //  testPointSystemTemperatureCompensationGain
      { 7,     1,     0xf,       0,     0,    0}, //  testPointSytemInputSelect
      { 4,     0,     0x7,       3,     0,    0}, //  testPulserCurrent
      { 2,     0,     0x3ff,     0,     0,    0}, //  testPulserLevel
      { 6,     0,     0x3f,      22,    0,    0}, //  VRefBaselineDac
    };

    static char _ARegNames[ASIC_ConfigV1::NumberOfRegisters+1][120] = {
    "RowStartAddr",
	"RowStopAddr",
	"ColumnStartAddr",
	"ColumnStopAddr",
	"chipID",
	"automaticTestModeEnable",
	"balconyDriverCurrent",
	"balconyEnable",
	"bandGapReferenceTemperatureCompensationBits",
	"CCK_RegDelayEnable",
	"digitalMonitor1Enable",
	"digitalMonitor2Enable",
	"digitalMonitorMux1",
	"digitalMonitorMux2",
	"dummyMask",
	"dummyTest",
	"EXEC_DelayEnable",
	"extraRowsLowReferenceValue",
	"fastPowerPulsingEnable",
	"fastPowerPulsingSpeed",
	"highResolutionModeTest",
	"interleavedReadOutDisable",
	"LVDS_ImpedenceMatchingEnable",
	"outputDriverDacReferenceBias",
	"outputDriverDrivingCapabilitiesAndStability",
	"outputDriverInputCommonMode0",
	"outputDriverInputCommonMode1",
	"outputDriverInputCommonMode2",
	"outputDriverInputCommonMode3",
	"outputDriverOutputDynamicRange0",
	"outputDriverOutputDynamicRange1",
	"outputDriverOutputDynamicRange2",
	"outputDriverOutputDynamicRange3",
	"outputDriverTemperatureCompensationEnable",
	"outputDriverTemperatureCompensationGain0",
	"outputDriverTemperatureCompensationGain1",
	"outputDriverTemperatureCompensationGain2",
	"outputDriverTemperatureCompensationGain3",
	"pixelBuffersAndPreamplifierDrivingCapabilities",
	"pixelFilterLevel",
	"pixelOutputBufferCurrent",
	"preamplifierCurrent",
	"programmableReadoutDelay",
	"pulserCounterDirection",
	"pulserReset",
	"pulserSync",
	"pulserVsPixelOnDelay",
	"syncPinEnable",
	"testBackEnd",
	"testMode",
	"testModeWithDarkFrame",
	"testPointSystemInputCommonMode",
	"testPointSystemOutputDynamicRange",
	"testPointSystemTemperatureCompensationEnable",
	"testPointSystemTemperatureCompensationGain",
	"testPointSytemInputSelect",
	"testPulserCurrent",
	"testPulserLevel",
	"VRefBaselineDac",
	{"--INVALID--"}        //    NumberOfRegisters
    };


    static Register* _Aregs = (Register*) _ASICfoo;
    static bool      namesAreInitialized = false;

    ASIC_ConfigV1::ASIC_ConfigV1(bool init) {
      if (init) {
        for (int i=0; i<NumberOfValues; i++) {
          _values[i] = 0;
        }
      }
    }

    ASIC_ConfigV1::~ASIC_ConfigV1() {}

    unsigned   ASIC_ConfigV1::get (ASIC_ConfigV1::Registers r) const {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::get parameter out of range!! %u\n", r);
        return 0;
      }
      return ((_values[_Aregs[r].offset] >> _Aregs[r].shift) & _Aregs[r].mask);
    }

    void   ASIC_ConfigV1::set (ASIC_ConfigV1::Registers r, unsigned v) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::set parameter out of range!! %u\n", r);
        return;
      }
      _values[_Aregs[r].offset] &= ~(_Aregs[r].mask << _Aregs[r].shift);
      _values[_Aregs[r].offset] |= (_Aregs[r].mask & v) << _Aregs[r].shift;
      return;
    }

    void   ASIC_ConfigV1::clear() {
      memset(_values, 0, NumberOfValues*sizeof(uint32_t));
    }

    uint32_t   ASIC_ConfigV1::rangeHigh(ASIC_ConfigV1::Registers r) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::rangeHigh parameter out of range!! %u\n", r);
        return 0;
      }
      return _Aregs[r].mask;
    }

    uint32_t   ASIC_ConfigV1::rangeLow(ASIC_ConfigV1::Registers) {
      return 0;
    }

    uint32_t   ASIC_ConfigV1::defaultValue(ASIC_ConfigV1::Registers r) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::defaultValue parameter out of range!! %u\n", r);
        return 0;
      }
      return _Aregs[r].defaultValue & _Aregs[r].mask;
    }

    char* ASIC_ConfigV1::name(ASIC_ConfigV1::Registers r) {
      return r < ASIC_ConfigV1::NumberOfRegisters ? _ARegNames[r] : _ARegNames[ASIC_ConfigV1::NumberOfRegisters];
    }

    void ASIC_ConfigV1::initNames() {
      static char range[60];
      if (namesAreInitialized == false) {
        for (unsigned i=0; i<NumberOfRegisters; i++) {
          Registers r = (Registers) i;
          sprintf(range, "  (%u->%u)    ", rangeLow(r), rangeHigh(r));
          if (_Aregs[r].readOnly != ReadOnly) {
            strncat(_ARegNames[r], range, 40);
//            printf("ASIC_ConfigV1::initNames %u %s %s\n", i, range, _ARegNames[r]);
          }
        }
      }
      namesAreInitialized = true;
    }

    unsigned ASIC_ConfigV1::readOnly(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::Epix100aSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return UseOnly;
      }
      return (unsigned)_Aregs[r].readOnly;
    }

    unsigned ASIC_ConfigV1::doNotCopy(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::Epix100aSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return DoNotCopy;
      }
      return (unsigned)_Aregs[r].doNotCopy;
    }

    void ASIC_ConfigV1::operator=(ASIC_ConfigV1& foo) {
      unsigned i=0;
      while (i<NumberOfRegisters) {
        Registers c = (Registers)i++;
        if ((readOnly(c) == ReadWrite) && (doNotCopy(c) == DoCopy)) set(c,foo.get(c));
        else printf("ASIC_ConfigV1::operator= did not copy %s because %s", name(c),
        		(readOnly(c) == ReadWrite) ? "(doNotCopy(c) != DoCopy)" : "(readOnly(c) != ReadWrite)");
      }
    }

    bool ASIC_ConfigV1::operator==(ASIC_ConfigV1& foo) {
      unsigned i=0;
      bool ret = true;
      while (i<NumberOfRegisters && ret) {
        Registers c = (Registers)i++;
        if (doNotCopy(c) == DoCopy) {
          // don't compare if not ReadWrite
          ret = ( (readOnly(c) != ReadWrite) || (get(c) == foo.get(c)));
          if (ret == false) {
            printf("\tEpix100a ASIC_ConfigV1 %u != %u at %s\n", get(c), foo.get(c), ASIC_ConfigV1::name(c));
          }
        }
      }
      return ret;
    }

  } /* namespace Epix100aConfig */
} /* namespace Pds */
