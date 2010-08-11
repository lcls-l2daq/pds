#include "pdsdata/cspad/ElementV1.hh"
namespace Pds {

  enum {compBias1=0xff, compBias2=0xc8, iss2=0x3c, iss5=0x25, vref=0xba};
  enum {rampcurrR1=0x4, rampCurrR2=0x25, rampCurrRef=0, rampVoltRef=0x61, analogPrst=0xfc};
  enum {vinj=0xba};
  enum {firmwareHeaderSizeInBytes=16};

  uint8_t myPots[80] = {
      vref, vref, rampcurrR1, 0,
      compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1,
      compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1,
      vref, vref, rampCurrR2, vinj,
      compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2,
      compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2,
      vref, vref, rampCurrRef, 0,
      iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2,
      vref, vref, rampVoltRef, analogPrst,
      iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5
      //    0,  1,  2,  3,  4,  5,  6,  7,
      //    8,  9, 10, 11, 12, 13, 14, 15,
      //   16, 17, 18, 19, 20, 21, 22, 23,
      //   24, 25, 26, 27, 28, 29, 30, 31,
      //   32, 33, 34, 35, 36, 37, 38, 39,
      //   40, 41, 42, 43, 44, 45, 46, 47,
      //   48, 49, 50, 51, 52, 53, 54, 55,
      //   56, 57, 58, 59, 60, 61, 62, 63,
      //   64, 65, 66, 67, 68, 69, 70, 71,
      //   72, 73, 74, 75, 76, 77, 78, 79
  };

  //shiftSelect    0x00000002
  //edgeSelect     0x00000001
  //readClkSet     0x00000002
  //readClkHold    0x00000001
  //dataMode       0x0
  //prstSel        0x00000000
  //acqDelay       0x00000118
  //intTime        0x000005dc
  //digDelay       0x000003c0
  //ampIdle        0x00000000
  //injTotal       0x00
  //rowColShiftPer 0x00000005
  //gainMap        0xffff

  uint32_t myQuad[13] = {2, 1, 2, 1, Pds::CsPad::testData, 0, 0x118, 0x5dc, 0x3c0, 0, 0, 5};

  class pGain {
    public:
      pGain() {
        for (int c=0; c<Pds::CsPad::ColumnsPerASIC; c++) {
          for (int r=0; r<Pds::CsPad::MaxRowsPerASIC; r++) {
            map[c][r] = 0xffff;
          }
        }
      }
    public:
      uint16_t map[Pds::CsPad::ColumnsPerASIC][Pds::CsPad::MaxRowsPerASIC];
  };

  class pPots {
    public:
      pPots() {
        for(int i=0; i<80; i++) pots[i] = myPots[i];
      };
    public:
      uint8_t pots[80];
  };

  class pReadOnly {
    public:
      pReadOnly() {
        ro[0] = ro[1] = 0;
      }
    public:
      uint32_t ro[2];
  };

  class pQuad {
    public:
      pQuad() {
        for(int i=0; i<12; i++) wr[i] = myQuad[i];
      }
    public:
      uint32_t wr[12];
      pReadOnly ro;
      pPots     pots;
      pGain     map;
  };

  class pCfg {
    public:
      pCfg() : rd(0), ec(40), arm(Pds::CsPad::RunAndSendTriggeredByTTL), tdi(4), ppq(sizeof(Pds::CsPad::ElementV1)), bam(0), am(0xf), qm(0xf) {};
    public:
      void testDataIndex(uint32_t i) {tdi=i;}
      void quadMask(uint32_t m) {qm=m;}

    public:
      uint32_t rd;
      uint32_t ec;
      uint32_t arm;
      uint32_t tdi;
      uint32_t ppq;
      uint32_t bam;
      uint32_t am;
      uint32_t qm;
      pQuad  quads[4];
  };
}

