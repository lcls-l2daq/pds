/*
 * Epix100aConfigV1.hh
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#ifndef EPIX100ACONFIGV1_HH_
#define EPIX100ACONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/Epix100aASICConfigV1.hh"
#include "pds/config/Epix100aConfigConstants.hh"
#include <stdint.h>



namespace Pds {
namespace Epix100aConfig_V1 {

class ASIC_ConfigV1;

class ConfigV1 {
public:
	ConfigV1(bool init=false);
	~ConfigV1();
	enum Constants {
		RowsPerAsic = RowsPerASIC,
		ColsPerAsic = ColsPerASIC,
		ASICsPerRow = AsicsPerRow,
		ASICsPerCol = AsicsPerCol,
		CalibrationRowCountPerAsic = 2
	};

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
		AcqToAsicR0Delay,
		AsicR0ToAsicAcq,
		AsicAcqWidth,
		AsicAcqLToPPmatL,
		AsicPPmatToReadout,
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
		NumberOfAsicsPerRow,
		NumberOfAsicsPerColumn,
		NumberOfRowsPerAsic,
		NumberOfReadableRowsPerAsic,
		NumberOfPixelsPerAsicRow,
		CalibrationRowCountPerASIC,
		EnvironmentalRowCountPerASIC,
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
		NumberOfValues=36
	};

	enum {
		NumberOfAsics=4, NumberOfASICs=4    //  Kludge ALERT!!!!!  this may need to be dynamic !!
	};

	unsigned                    get      (Registers) const;
	void                        set      (Registers, unsigned);
	void                        clear();
	Pds::Epix100aConfig::ASIC_ConfigV1*              asics() { return (Pds::Epix100aConfig::ASIC_ConfigV1*) (this+1); }
	const Pds::Epix100aConfig::ASIC_ConfigV1*        asics() const { return (const Pds::Epix100aConfig::ASIC_ConfigV1*) (this+1); }
	uint32_t                    numberOfAsics() const;
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
	//        Pds::Epix100aConfig::ASIC_ConfigV1 _asics[NumberOfAsics];
	//        uint32_t     _pixelTestArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
	//        uint32_t     _pixelMaskArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
};

} /* namespace Epix100aConfig_V1 */
} /* namespace Pds */

#endif /* EPIX100ACONFIGV1_HH_ */
