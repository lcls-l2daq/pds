/*
 * EpixASICConfigV1.cc
 *
 *  Created on: Nov 6, 2013
 *      Author: jackp
 */

#include "EpixASICConfigV1.hh"

namespace Pds {
  namespace EpixConfig {

    class Register {
      public:
        uint32_t offset;
        uint32_t shift;
        uint32_t mask;
        uint32_t defaultValue;
        ASIC_ConfigV1::readOnlyStates readOnly;
    };

    static uint32_t _ASICfoo[][5] = {
 //offset shift mask default
        {0,  0,  7,      7,     0},     // monostPulser,
        {1,  0,  1,      0,     0},     // dummyTest,
        {1,  1,  1,      0,     0},     // dummyMask,
        {2,  0,  0x3ff,  0,     0},     // pulser,
        {2,  10, 1,      0,     0},     // pbit,
        {2,  11, 1,      0,     0},     // atest,
        {2,  12, 1,      0,     0},     // test,
        {2,  13, 1,      0,     0},     // sabTest,
        {2,  14, 1,      0,     0},     // hrTest,
        {3,  0,  0xf,    0,     0},     // digMon1,
        {3,  4,  0xf,    1,     0},     // digMon2,
        {4,  0,  7,      3,     0},     // pulserDac,
        {5,  0,  1,      0,     0},     // Dm1En,
        {5,  1,  1,      0,     0},     // Dm2En,
        {5,  4,  1,      0,     0},     // slvdSBit,
        {6,  0,  0x3f,   0x1e,  0},     // VRefDac,
        {7,  0,  1,      1,     0},     // TpsTComp,
        {7,  1,  0xf,    7,     0},     // TpsMux,
        {7,  5,  7,      3,     0},     // RoMonost,
        {8,  0,  0xf,    0x4,   0},     // TpsGr,
        {8,  4,  0xf,    0x4,   0},     // S2dGr,
        {9,  0,  1,      1,     0},     // PpOcbS2d,
        {9,  1,  7,      3,     0},     // Ocb,
        {9,  4,  7,      3,     0},     // Monost,
        {9,  7,  1,      1,     0},     // FastppEnable,
        {10, 0,  7,      7,     0},     // Preamp,
        {10, 4,  7,      4,     0},     // PixelCb,
        {11, 0,  1,      1,     0},     // S2dTComp,
        {11, 1,  0x3f,   0,     0},     // FilterDac,
        {12, 0,  3,      0,     0},     // TC,
        {12, 2,  7,      3,     0},     // S2d,
        {12, 5,  7,      3,     0},     // S2dDacBias,
        {13, 0,  3,      1,     0},     // TpsTcDac,
        {13, 2,  0x3f,   0x1c,  0},     // TpsDac,
        {14, 0,  3,      1,     0},     // S2dTcDac,
        {14, 2,  0x3f,   0x1c,  0},     // S2dDac,
        {15, 0,  1,      0,     0},     // testBE,
        {15, 1,  1,      0,     0},     // IsEn,
        {15, 2,  1,      0,     0},     // DelExec,
        {15, 3,  1,      0,     0},     // DelCckReg,
        {16, 0,  0x1ff,  0,     3},     // RowStartAddr,
        {17, 0,  0x1ff,  97,    3},     // RowStopAddr,
        {18, 0,  0x7f,   0,     3},     // ColStartAddr,
        {19, 0,  0x7f,   95,    3},     // ColStopAddr,
        {20, 0,  0xffff, 0,     1}      // chipID,
    };

    static char _ARegNames[ASIC_ConfigV1::NumberOfRegisters+1][120] = {
        {"monostPulser"},
        {"dummyTest"},
        {"dummyMask"},
        {"pulser"},
        {"pbit"},
        {"atest"},
        {"test"},
        {"sabTest"},
        {"hrTest"},
        {"digMon1"},
        {"digMon2"},
        {"pulserDac"},
        {"Dm1En"},
        {"Dm2En"},
        {"slvdSBit"},
        {"VRefDac"},
        {"TpsTComp"},
        {"TpsMux"},
        {"RoMonost"},
        {"TpsGr"},
        {"S2dGr"},
        {"PpOcbS2d"},
        {"Ocb"},
        {"Monost"},
        {"FastppEnable"},
        {"Preamp"},
        {"PixelCb"},
        {"S2dTComp"},
        {"FilterDac"},
        {"TC"},
        {"S2d"},
        {"S2dDacBias"},
        {"TpsTcDac"},
        {"TpsDac"},
        {"S2dTcDac"},
        {"S2dDac"},
        {"testBE"},
        {"IsEn"},
        {"DelExec"},
        {"DelCckReg"},
        {"RowStartAddr"},
        {"RowStopAddr"},
        {"ColStartAddr"},
        {"ColStopAddr"},
        {"chipID"},
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
          sprintf(range, "  (0x%x->0x%x)    ", rangeLow(r), rangeHigh(r));
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
        printf("Pds::EpixSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return false;
      }
      return (unsigned)_Aregs[r].readOnly;
    }

    void ASIC_ConfigV1::operator=(ASIC_ConfigV1& foo) {
      unsigned i=0;
      while (i<NumberOfRegisters) {
        Registers c = (Registers)i++;
        if (readOnly(c) == ReadWrite) set(c,foo.get(c));
      }
    }

    bool ASIC_ConfigV1::operator==(ASIC_ConfigV1& foo) {
      unsigned i=0;
      bool ret = true;
      while (i<NumberOfRegisters && ret) {
        Registers c = (Registers)i++;
        ret = ((get(c) == foo.get(c)) || (readOnly(c) != ReadWrite));
        if (ret == false) {
          printf("\tEpix ASIC_ConfigV1 %u != %u at %s\n", get(c), foo.get(c), ASIC_ConfigV1::name(c));
        }
      }
      return ret;
    }

  } /* namespace EpixConfig */
} /* namespace Pds */
