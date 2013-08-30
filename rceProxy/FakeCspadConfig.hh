#include "pdsdata/psddl/cspad.ddl.h"
namespace Pds {

  enum {compBias1=0xff, compBias2=0xc8, iss2=0x3c, iss5=0x25, vref=0xba};
  enum {rampcurrR1=0x4, rampCurrR2=0x25, rampCurrRef=0, rampVoltRef=0x61, analogPrst=0xfc};
  enum {vinj=0xba};
  enum {firmwareHeaderSizeInBytes=16};
  enum { ASICS=16, Columns=185, Rows=194 };

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

  uint32_t myQuad[18] = {
      4, 4, 4, 4,  //shiftSelect
      0, 0, 0, 0, //edgeSelect
      2, //readClkSet
      1, //readClkHold
      0, //dataMode
      1, //prstSel
      0x118, //acqDelay
      0x5dc, //intTime
      0x3c0, //digDelay
      0,  //ampIdle
      0, //injTotal
      5 //rowColShiftPer
  };

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
        for(unsigned i=0; i<sizeof(myPots); i++) pots[i] = myPots[i];
      };
    public:
      uint8_t pots[sizeof(myPots)];
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
        for(unsigned i=0; i<(sizeof(myQuad)/sizeof(uint32_t)); i++) wr[i] = myQuad[i];
      }
    public:
      uint32_t wr[sizeof(myQuad)/sizeof(uint32_t)];
      pReadOnly ro;
      pPots     pots;
      pGain     map;
  };

  class pCfg {
    public:
      pCfg() :
        rd(2),    // runDelay
        ec(40),   // eventCode (40-45)
        irm(Pds::CsPad::RunButDrop), // inactiveRunMode (1,5)
        // activeRunMode (2,3,4)
        arm(Pds::CsPad::RunAndSendTriggeredByTTL),
        tdi(4),  // testDataIndex (0-7)
        // payloadPerQuad
        ppq(sizeof(Pds::CsPad::ElementV1) + ASICS*Columns*Rows*sizeof(uint16_t) + 4),
        bam(0),  // badAsicMask (?)
        am(0xf), // asicMask (1-15)
        qm(0xf) {}; // quadMask (1-15)
    public:
      void testDataIndex(uint32_t i) {tdi=i;}
      void quadMask(uint32_t m) {qm=m;}

    public:
      uint32_t rd;
      uint32_t ec;
      uint32_t irm;
      uint32_t arm;
      uint32_t tdi;
      uint32_t ppq;
      uint64_t bam;
      uint32_t am;
      uint32_t qm;
      pQuad  quads[4];
  };
}

