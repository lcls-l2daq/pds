/*
 * EpixSamplerConfig.cc
 *
 *  Created on: Nov 5, 2013
 *      Author: jackp
 */

#include "pds/config/EpixSamplerConfigV1.hh"

namespace Pds {
  namespace EpixSamplerConfig {

    class  Register {
      public:
        uint32_t                 offset;
        uint32_t                 shift;
        uint32_t                 mask;
        uint32_t                 defaultValue;
        ConfigV1::readOnlyStates readOnly;
    } ;


    static uint32_t _regsfoo[ConfigV1::NumberOfRegisters][5] = {
    //offset shift mask default readOnly
        {  0, 0, 0xffffffff, 0,     1},    //  Version
        {  1, 0, 0xffffffff, 0,     0},    //  RunTrigDelay
        {  2, 0, 0xffffffff, 0,     0},    //  DaqTrigDelay
        {  3, 0, 0xffffffff, 0,     0},    //  DaqSetting
        {  4, 0, 0x3ff,      2,     0},    //  AdcClkHalfT
        {  5, 0, 0xffffffff, 0x11,  0},    //  AdcPipelineDelay
        {  6, 0, 0xffffffff, 0,     1},    //  DigitalCardId0
        {  7, 0, 0xffffffff, 0,     1},    //  DigitalCardId1
        {  8, 0, 0xffffffff, 0,     1},    //  AnalogCardId0
        {  9, 0, 0xffffffff, 0,     1},    //  AnalogCardId1
        { 10, 0, 0xff,       8,     2},    //  NumberOfChannels
        { 11, 0, 0xffff,     8192,  2},    //  SamplesPerChannel
        { 12, 0, 0xffffffff, 0,     1},    //  BaseClockFrequency
        { 13, 8, 0x1,        0,     0}     //  TestPatternEnable
    };


    static Register* _regs = (Register*) _regsfoo;
    static bool      namesAreInitialized = false;

    ConfigV1::ConfigV1(bool init) {
      if (!namesAreInitialized){
        int r;
        for (r=0; r< (int)NumberOfRegisters; r++) {
          name((Registers)r, true);
        }
        namesAreInitialized = true;
      }
      if (init) {
        for (int i=0; i<NumberOfValues; i++) {
          _values[i] = 0;
        }
      }
    }

    ConfigV1::~ConfigV1() {}

    unsigned   ConfigV1::get (Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::EpixSamplerConfig::ConfigV1::get parameter out of range!! %u\n", r);
        return 0;
      }
      return ((_values[_regs[r].offset] >> _regs[r].shift) & _regs[r].mask);
    }

    void   ConfigV1::set (Registers r, unsigned v) {
      if (r >= NumberOfRegisters) {
        printf("Pds::EpixSamplerConfig::ConfigV1::set parameter out of range!! %u\n", r);
        return;
      }
      _values[_regs[r].offset] &= ~(_regs[r].mask << _regs[r].shift);
      _values[_regs[r].offset] |= (_regs[r].mask & v) << _regs[r].shift;
      return;
    }

    uint32_t   ConfigV1::rangeHigh(Registers r) {
      uint32_t ret = _regs[r].mask;
      if (r >= NumberOfRegisters) {
        printf("Pds::EpixSamplerConfig::ConfigV1::rangeHigh parameter out of range!! %u\n", r);
        return 0;
      }
      switch (r) {
        case AdcClkHalfT :
          ret = 400;
          break;
        case NumberOfChannels :
          ret = 8;
          break;
        case SamplesPerChannel :
          ret = 8192;
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
          ret = 2;
          break;
        case NumberOfChannels :
          ret = 8;
          break;
        case SamplesPerChannel :
          ret = 8192;
          break;
        default:
          break;
      }
      return ret;
    }

    uint32_t   ConfigV1::defaultValue(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::EpixSamplerConfig::ConfigV1::defaultValue parameter out of range!! %u\n", r);
        return 0;
      }
      return _regs[r].defaultValue & _regs[r].mask;
    }

    char* ConfigV1::name(Registers r, bool init) {
      static char _regNames[NumberOfRegisters+1][120] = {
          {"Version            "},
          {"RunTrigDelay       "},
          {"DaqTrigDelay       "},
          {"DaqSetting         "},
          {"AdcClkHalfT        "},
          {"AdcPipelineDelay   "},
          {"DigitalCardId0     "},
          {"DigitalCardId1     "},
          {"AnalogCardId0      "},
          {"AnalogCardId1      "},
          {"NumberOfChannels   "},
          {"SamplesPerChannel  "},
          {"BaseCLockFrequency "},
          {"testPatternEnable  "},
          {"-----INVALID-------"}    //    NumberOfRegisters
      };
      static char range[60];
      if (init && (r < NumberOfRegisters)) {
        sprintf(range, "  (0x%x->0x%x)    ", rangeLow(r), rangeHigh(r));
        strncat(_regNames[r], range, 40);
      }
      return r < NumberOfRegisters ? _regNames[r] : _regNames[NumberOfRegisters];
    }

    unsigned ConfigV1::readOnly(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::EpixSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return false;
      }
      return (unsigned)_regs[r].readOnly;
    }
  } /* namespace EpixSamplerConfig */
} /* namespace Pds */
