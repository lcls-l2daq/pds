/*
 * EpixConfigV1.cc
 *
 *  Created on: Nov 5, 2013
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/config/EpixConfigV1.hh"
#include "pds/config/EpixASICConfigV1.hh"

namespace Pds {
  namespace EpixConfig {

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
        {  0, 0, 0xffffffff, 0,      1},    //  Version,
        {  1, 0, 0xffffffff, 0,      0},    //  RunTrigDelay,
        {  2, 0, 0xffffffff, 0x4e2,  2},    //  DaqTrigDelay,
        {  3, 0, 0xffffffff, 0x4e20, 0},    //  DacSetting,
        //  pin states(4) and controls(5)
        {  4, 0, 1, 1, 0},    //  asicGR,
        {  5, 0, 1, 1, 0},    //  asicGRControl,
        {  4, 1, 1, 0, 0},    //  asicAcq,
        {  5, 1, 1, 0, 0},    //  asicAcqControl,
        {  4, 2, 1, 1, 0},    //  asicR0,
        {  5, 2, 1, 0, 0},    //  asicR0Control,
        {  4, 3, 1, 1, 0},    //  asicPpmat,
        {  5, 3, 1, 1, 0},    //  asicPpmatControl,
        {  4, 4, 1, 0, 0},    //  asicPpbe,
        {  5, 4, 1, 0, 0},    //  asicPpbeControl,
        {  4, 5, 1, 0, 0},    //  asicRoClk,
        {  5, 5, 1, 0, 0},    //  asicR0ClkControl,
        {  5, 6, 1, 1, 0},    //  prepulseR0En,
        {  5, 7, 1, 0, 0},    //  adcStreamMode,
        {  5, 8, 1, 0, 0},    //  testPatternEnable,
        //
        {  6, 0, 0xffffffff, 0xea6,      0},    //  AcqToAsicR0Delay,
        {  7, 0, 0xffffffff, 0xfa0,      0},    //  AsicR0ToAsicAcq,
        {  8, 0, 0xffffffff, 0xfa0,      0},    //  AsicAcqWidth,
        {  9, 0, 0xffffffff, 0xfa,       0},    //  AsicAcqLToPPmatL,
        { 10, 0, 0xffffffff, 0xd,        0},    //  AsicRoClkHalfT,
        { 11, 0, 3,          1,          0},    //  AdcReadsPerPixel,
        { 12, 0, 0xffffffff, 2,          0},    //  AdcClkHalfT,
        { 13, 0, 0xffffffff, 0x26,       0},    //  AsicR0Width,
        { 14, 0, 0xffffffff, 0x11,       0},    //  AdcPipelineDelay,
        { 15, 0, 0xffffffff, 0x26,       0},    //  PrepulseR0Width,
        { 16, 0, 0xffffffff, 0x9c40,     0},    //  PrepulseR0Delay,
        { 17, 0, 0xffffffff, 0,          1},    //  DigitalCardId0,
        { 18, 0, 0xffffffff, 0,          1},    //  DigitalCardId1,
        { 19, 0, 0xffffffff, 0,          1},    //  AnalogCardId0,
        { 20, 0, 0xffffffff, 0,          1},    //  AnalogCardId1,
        { 21, 0, 0xff,       0,          2},    //  LastRowExclusions,
        { 22, 0, 0xf,        2,          2},    //  NumberOfAsicsPerRow,
        { 23, 0, 0xf,        2,          2},    //  NumberOfAsicsPerColumn,
        { 24, 0, 0x1ff,      352,        2},    //  NumberOfRowsPerAsic,
        { 25, 0, 0x1ff,      384,        2},    //  NumberOfPixelsPerAsicRow,
        { 26, 0, 0xffffffff, 0,          1},    //  BaseClockFrequency,
        { 27, 0, 0xffff,     0xf,        0},    //  AsicMask,
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
        {"AcqToAsicR0Delay"},
        {"AsicR0ToAsicAcq"},
        {"AsicAcqWidth"},
        {"AsicAcqLToPPmatL"},
        {"AsicRoClkHalfT"},
        {"AdcReadsPerPixel"},
        {"AdcClkHalfT"},
        {"AsicR0Width"},
        {"AdcPipelineDelay"},
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
        {"-------INVALID------------"}//NumberOfRegisters
    };

    static Register* _regs = (Register*) _regsfoo;
    static bool namesAreInitialized = false;

    ConfigV1::ConfigV1(bool init) {
      if (init) {
        for (int i=0; i<NumberOfValues; i++) {
          _values[i] = 0;
        }
      }
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

    uint32_t ConfigV1::offset(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV1::set parameter out of range!! %u\n", r);
        return 0xffffffff;
      }
      return _regs[r].offset;
    }

    uint32_t ConfigV1::numberOfAsics() {
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
        case AdcReadsPerPixel:
        case AsicMask :
          ret = 1;
          break;
        case AdcClkHalfT :
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
        printf("EpixConfigV1::readOnly parameter out of range!! %u\n", r);
        return 400;
      }
      return (unsigned)_regs[r].readOnly;
    }
  } /* namespace EpixConfig */
} /* namespace Pds */
