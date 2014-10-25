/*
 * Epix10kConfigV1.cc
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/config/Epix10kConfigV1.hh"
#include "pds/config/Epix10kASICConfigV1.hh"

namespace Pds {
  namespace Epix10kConfig {

    class  Register {
      public:
        uint32_t offset;
        uint32_t shift;
        uint32_t mask;
        uint32_t defaultValue;
        uint32_t readOnly;
    } ;

    static uint32_t _regsfoo[ConfigV1::NumberOfRegisters][5] = {
    //offset shift mask default readOnly
        {  0,  0, 0xffffffff, 0,      1},    //  Version,
        {  1,  0, 0xffffffff, 0,      0},    //  RunTrigDelay,
        {  2,  0, 0xffffffff, 0x4e2,  2},    //  DaqTrigDelay,
        {  3,  0, 0xffffffff, 0x4e20, 0},    //  DacSetting,
        //  pin states(4) and controls(5)
        {  4,  0, 1, 1, 0},    //  asicGR,
        {  5,  0, 1, 1, 0},    //  asicGRControl,
        {  4,  1, 1, 0, 0},    //  asicAcq,
        {  5,  1, 1, 0, 0},    //  asicAcqControl,
        {  4,  2, 1, 1, 0},    //  asicR0,
        {  5,  2, 1, 0, 0},    //  asicR0Control,
        {  4,  3, 1, 1, 0},    //  asicPpmat,
        {  5,  3, 1, 1, 0},    //  asicPpmatControl,
        {  4,  4, 1, 0, 0},    //  asicPpbe,
        {  5,  4, 1, 0, 0},    //  asicPpbeControl,
        {  4,  5, 1, 0, 0},    //  asicRoClk,
        {  5,  5, 1, 0, 0},    //  asicR0ClkControl,
        {  5,  6, 1, 1, 0},    //  prepulseR0En,
        {  5,  7, 1, 0, 0},    //  adcStreamMode,
        {  5,  8, 1, 0, 0},    //  testPatternEnable,
        {  5,  9, 3, 0, 0},    //  SyncMode
        {  5, 11, 1, 0, 0},    //  R0Mode
        //
        {  6,  0, 0xffffffff, 0xea6,      0},    //  DoutPipeLineDelay,
        {  7,  0, 0xffffffff, 0xea6,      0},    //  AcqToAsicR0Delay,
        {  8,  0, 0xffffffff, 0xfa0,      0},    //  AsicR0ToAsicAcq,
        {  9,  0, 0xffffffff, 0xfa0,      0},    //  AsicAcqWidth,
        { 10,  0, 0xffffffff, 0xfa,       0},    //  AsicAcqLToPPmatL,
        { 11,  0, 0xffffffff, 0xd,        0},    //  AsicRoClkHalfT,
        { 12,  0, 3,          1,          0},    //  AdcReadsPerPixel,
        { 13,  0, 0xffffffff, 2,          0},    //  AdcClkHalfT,
        { 14,  0, 0xffffffff, 0x26,       0},    //  AsicR0Width,
        { 15,  0, 0xffffffff, 0x11,       0},    //  AdcPipelineDelay,
        { 16,  0, 0xffff,     0x26,       0},    //  SnycWidth,
        { 16, 16, 0xffff,     0x26,       0},    //  SyncDelay,
        { 17,  0, 0xffffffff, 0x26,       0},    //  PrepulseR0Width,
        { 18,  0, 0xffffffff, 0x9c40,     0},    //  PrepulseR0Delay,
        { 19,  0, 0xffffffff, 0,          1},    //  DigitalCardId0,
        { 20,  0, 0xffffffff, 0,          1},    //  DigitalCardId1,
        { 21,  0, 0xffffffff, 0,          1},    //  AnalogCardId0,
        { 22,  0, 0xffffffff, 0,          1},    //  AnalogCardId1,
        { 23,  0, 0xff,       0,          2},    //  LastRowExclusions,
        { 24,  0, 0xf,        2,          2},    //  NumberOfAsicsPerRow,
        { 25,  0, 0xf,        2,          2},    //  NumberOfAsicsPerColumn,
        { 26,  0, 0x1ff,      51,        2},    //  NumberOfRowsPerAsic,
        { 27,  0, 0x1ff,      192,        2},    //  NumberOfPixelsPerAsicRow,
        { 28,  0, 0xffffffff, 0,          1},    //  BaseClockFrequency,
        { 29,  0, 0xffff,     0xf,        0},    //  AsicMask,
        { 30,  0, 1,          0,          0},    //  ScopeEnable,
        { 30,  1, 1,          1,          0},    //  ScopeTrigEdge,
        { 30,  2, 0xf,        6,          0},    //  ScopeTrigCh,
        { 30,  6, 3,          2,          0},    //  ScopeArmMode,
        { 30, 16, 0xffff,     0,          0},    //  ScopeAdcThresh,
        { 31,  0, 0x1fff,     0,          0},    //  ScopeHoldoff,
        { 31, 13, 0x1fff,     0xf,        0},    //  ScopeOffset,
        { 32,  0, 0x1fff,     0x1000,     0},    //  ScopeTraceLength,
        { 32, 13, 0x1fff,     0,          0},    //  ScopeSkipSamples,
        { 33,  0, 0x1f,       0,          0},    //  ScopeInputA,
        { 33,  5, 0x1f,       4,          0},    //  ScopeInputB,
    };

    static char _regNames[ConfigV1::NumberOfRegisters+1][120] = {
        {"Version"},
        {"RunTrigDelay"},
        {"DaqTrigDelay"},
        {"DacSetting"},
        {"asicGR"},
        {"asicGRControl"},
        {"asicAcq"},
        {"asicAcqControl"},
        {"asicR0"},
        {"asicR0Control"},
        {"asicPpmat"},
        {"asicPpmatControl"},
        {"asicPpbe"},
        {"asicPpbeControl"},
        {"asicRoClk"},
        {"asicRoClkControl"},
        {"prepulseR0En"},
        {"adcStreamMode"},
        {"testPatternEnable"},
        {"SyncMode"},
        {"R0Mode"},
        {"DoutPipeLineDelay"},
        {"AcqToAsicR0Delay"},
        {"AsicR0ToAsicAcq"},
        {"AsicAcqWidth"},
        {"AsicAcqLToPPmatL"},
        {"AsicRoClkHalfT"},
        {"AdcReadsPerPixel"},
        {"AdcClkHalfT"},
        {"AsicR0Width"},
        {"AdcPipelineDelay"},
        {"SyncWidth"},
        {"SyncDelay"},
        {"PrepulseR0Width"},
        {"PrepulseR0Delay"},
        {"DigitalCardId0"},
        {"DigitalCardId1"},
        {"AnalogCardId0"},
        {"AnalogCardId1"},
        {"LastRowExclusions"},
        {"NumberOfAsicsPerRow"},
        {"NumberOfAsicsPerColumn"},
        {"NumberOfRowsPerAsic"},
        {"NumberOfPixelsPerAsicRow"},
        {"BaseClockFrequency"},
        {"AsicMask"},
        {"ScopeEnable"},
        {"ScopeTrigEdge"},
        {"ScopeTrigCh"},
        {"ScopeArmMode"},
        {"ScopeAdcThresh"},
        {"ScopeHoldoff"},
        {"ScopeOffset"},
        {"ScopeTraceLength"},
        {"ScopeSkipSamples"},
        {"ScopeInputA"},
        {"ScopeInputB"},
        {"-------INVALID------------"}//NumberOfRegisters
    };

    static uint32_t _configAddrs[ConfigV1::NumberOfValues][2] = {
      {0x00,  1}, //  version
      {0x02,  0}, //  RunTrigDelay
      {0x04,  2}, //  DaqTrigDelay
      {0x07,  0}, //  DacSetting
      {0x29,  0}, //  AsicPins, etc
      {0x2a,  0}, //  AsicPinControl, etc
      {0x1f,  0}, //  DoutPipeLineDelay
      {0x20,  0}, //  AcqToAsicR0Delay
      {0x21,  0}, //  AsicR0ToAsicAcq
      {0x22,  0}, //  AsicAcqWidth
      {0x23,  0}, //  AsicAcqLToPPmatL
      {0x24,  0}, //  AsicRoClkHalfT
      {0x25,  0}, //  AdcReadsPerPixel
      {0x26,  0}, //  AdcClkHalfT
      {0x2b,  0}, //  AsicR0Width
      {0x2c,  0}, //  AdcPipelineDelay
      {0x2d,  0}, //  SyncParams
      {0x2e,  0}, //  PrepulseR0Width
      {0x2f,  0}, //  PrepulseR0Delay
      {0x30,  1}, //  DigitalCardId0
      {0x31,  1}, //  DigitalCardId1
      {0x32,  1}, //  AnalogCardId0
      {0x33,  1}, //  AnalogCardId1
      {0,     2}, //  LastRowExclusions
      {0,     2}, //  NumberOfAsicsPerRow
      {0,     2}, //  NumberOfAsicsPerColumn
      {0,     2}, //  NumberOfRowsPerAsic
      {0,     2}, //  NumberOfPixelsPerAsicRow
      {0x10,  1}, //  BaseClockFrequency
      {0xd,   0}, //  AsicMask
      {0x52,  0}, //  ScopeSetup1
      {0x53,  0}, //  ScopeSetup2
      {0x54,  0}, //  ScopeLengthAndSkip
      {0x55,  0}, //  ScopeInputSelects
    };

    static Register* _regs = (Register*) _regsfoo;
    static bool namesAreInitialized = false;

    ConfigV1::ConfigV1(bool init) {
      if (init) 
	clear();
    }

    ConfigV1::~ConfigV1() {}

    unsigned   ConfigV1::get (Registers r) const {
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::get parameter out of range!! %u\n", r);
        return 0;
      }
      return ((_values[_regs[r].offset] >> _regs[r].shift) & _regs[r].mask);
    }

    void   ConfigV1::set (Registers r, unsigned v) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::set parameter out of range!! %u\n", r);
        return;
      }
      _values[_regs[r].offset] &= ~(_regs[r].mask << _regs[r].shift);
      _values[_regs[r].offset] |= (_regs[r].mask & v) << _regs[r].shift;
      return;
    }

    void   ConfigV1::clear() {
      memset(_values, 0, NumberOfValues*sizeof(uint32_t));
      for(unsigned i=0; i<NumberOfAsics; i++)
	asics()[i].clear();
    }

    uint32_t ConfigV1::offset(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::set parameter out of range!! %u\n", r);
        return 0xffffffff;
      }
      return _regs[r].offset;
    }

    uint32_t ConfigV1::numberOfAsics() const {
      return get(NumberOfAsicsPerRow) * get(NumberOfAsicsPerColumn);
    }

    uint32_t   ConfigV1::rangeHigh(Registers r) {
      uint32_t ret = _regs[r].mask;
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::rangeHigh parameter out of range!! %u\n", r);
        return 0;
      }
      switch (r) {
        case AdcClkHalfT :
          ret = 400;
          break;
        case NumberOfAsicsPerRow :
        case NumberOfAsicsPerColumn :
          ret = 2;
          break;
        case DaqTrigDelay :
          ret = 1250;
          break;
        default:
          break;
      }
      return ret;
    }

    uint32_t   ConfigV1::rangeLow(Registers r) {
      uint32_t ret = 0;
      switch (r) {
        case AdcClkHalfT :
        case AdcReadsPerPixel:
        case AsicMask :
          ret = 1;
          break;
        case NumberOfAsicsPerRow :
        case NumberOfAsicsPerColumn :
          ret = 2;
          break;
        case DaqTrigDelay :
          ret = 1250;
          break;
        default:
          break;
      }
      return ret;
    }

    uint32_t   ConfigV1::defaultValue(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::defaultValue parameter out of range!! %u\n", r);
        return 0;
      }
      return _regs[r].defaultValue & _regs[r].mask;
    }

    char* ConfigV1::name(Registers r) {
      return r < NumberOfRegisters ? _regNames[r] : _regNames[NumberOfRegisters];
    }

    void ConfigV1::initNames() {
      static char range[60];
      if (namesAreInitialized == false) {
        for (unsigned i=0; i<NumberOfRegisters; i++) {
          Registers r = (Registers) i;
          sprintf(range, "  (0x%x->0x%x)    ", rangeLow(r), rangeHigh(r));
          if (_regs[r].readOnly != ReadOnly) {
            strncat(_regNames[r], range, 40);
//            printf("ConfigV1::initNames %u %s %s\n", i, range, _regNames[r]);
          }
        }
        ASIC_ConfigV1::initNames();
      } else {
//        printf("ConfigV1::initNames namesAreInitialized=%s\n", namesAreInitialized ? "true" : "false");
      }
      namesAreInitialized = true;
    }

    unsigned ConfigV1::readOnly(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Epix10kConfigV1::readOnly parameter out of range!! %u\n", r);
        return 400;
      }
      return (unsigned)_regs[r].readOnly;
    }

    unsigned ConfigV1::address(unsigned i) 
    { return _configAddrs[i][0]; }

    unsigned ConfigV1::mask(unsigned i) 
    { 
      unsigned m=0;
      for(unsigned j=0; j<NumberOfRegisters; j++)
        if (_regs[j].offset == i)
          m |= _regs[j].mask << _regs[j].shift;
      return m;
    }

    ConfigV1::readOnlyStates  ConfigV1::use(unsigned i)
    { return ConfigV1::readOnlyStates(_configAddrs[i][1]); }

  } /* namespace Epix10kConfig */
} /* namespace Pds */
