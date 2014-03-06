#include <stdio.h>
#include "pds/config/ImpConfigType.hh"

typedef struct {
  uint32_t mask;
  uint32_t defaultValue;
} RegisterV1;

static const RegisterV1 _regs[ImpConfigType::NumberOfRegisters] = {
  // mask           default
  { 0xffff,        0x0       },   //    Gain_range
  { 0xffff,        0x5555    },   //    Cal_range
  { 0xffffffff,    0x98968200},   //    Reset
  { 0xffff,        0x2000    },   //    Bias_data
  { 0xffff,        0x2fb5    },   //    Cal_data
  { 0xffff,        0xf000    },   //    BiasDac_data
  { 0xffff,        0x927c    },   //    Cal_strobe
  { 0x3ff,         0x3ff     },   //    NumberOfSamples
  { 0xffff,        0x0       },   //    TrigDelay
  { 0xffff,        0x80      }    //    Adc_delay
};

unsigned Pds::ImpConfig::get(const ImpConfigType& c,
                             ImpConfigType::Registers r)
{
  if (r >= ImpConfigType::NumberOfRegisters) {
    printf("ConfigV1::get parameter out of range!! %u\n", r);
    return 0;
  }
  const uint32_t* u = (const uint32_t*)(&c);
  return ((u[r]) & _regs[r].mask);
}

void     Pds::ImpConfig::set(ImpConfigType& c,
                             ImpConfigType::Registers r,
                             unsigned v)
{
  if (r >= ImpConfigType::NumberOfRegisters) {
    printf("ConfigV1::set parameter out of range!! %u\n", r);
  }
  uint32_t* u = (uint32_t*)(&c);
  u[r] = _regs[r].mask & v;
}

uint32_t   Pds::ImpConfig::rangeHigh(ImpConfigType::Registers r) {
  if (r >= ImpConfigType::NumberOfRegisters) {
    printf("ConfigV1::rangeHigh parameter out of range!! %u\n", r);
    return 0;
  }
  return _regs[r].mask;
}

uint32_t   Pds::ImpConfig::rangeLow(ImpConfigType::Registers r) {
  if (r==ImpConfigType::NumberOfSamples) {
    return 1;
  }
  return 0;
}

uint32_t   Pds::ImpConfig::defaultValue(ImpConfigType::Registers r) {
  if (r >= ImpConfigType::NumberOfRegisters) {
    printf("ConfigV1::defaultValue parameter out of range!! %u\n", r);
    return 0;
  }
  return _regs[r].defaultValue & _regs[r].mask;
}

const char*   const   Pds::ImpConfig::name     (ImpConfigType::Registers r) {
  static char _regsNames[ImpConfigType::NumberOfRegisters+1][120] = {
    {"Gain_range"},         //      Gain_range
    {"Cal_range"},          //      Cal_range
    {"Reset"},              //      Reset
    {"Bias_data"},          //      Bias_data
    {"Cal_data"},           //      Cal_data
    {"BiasDac_data"},       //      BiasDac_data
    {"Cal_strobe"},         //      Cal_strobe
    {"NumberOfSamples"},    //      NumberOfSamples
    {"TrigDelay"},          //      TrigDelay
    {"AdcDelay"},           //      AdcDelay
    {"----INVALID----"}       //      NumberOfRegisters
  };
  static char range[60];

  static bool init=false;
  if (!init) {
    init=true;
    for(unsigned i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      ImpConfigType::Registers ir = (ImpConfigType::Registers)i;
      sprintf(range, "  (%u..%u)    ", 0, _regs[ir].mask);
      strncat(_regsNames[ir], range, 40);
    }
  }

  return r < ImpConfigType::NumberOfRegisters ? _regsNames[r] : _regsNames[ImpConfigType::NumberOfRegisters];

};
