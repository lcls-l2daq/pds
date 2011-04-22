/*
 * DataImportFrame.hh
 *
 *  Created on: Jun 8, 2010
 *      Author: jackp
 */

#ifndef DATAIMPORTFRAME_HH_
#define DATAIMPORTFRAME_HH_

//   Header = 8 x 32-bits
//       Header[0] = Tid[23:0], Lane[1:0], Z[3:0], VC[1:0]
//       Header[1] = Z[5:0], Quad[1:0], OpCode[7:0], acqCount[15:0]
//       Header[2] = BeamCode[31:0]
//       Header[3] = Z[31:0]
//       Header[4] = Z[31:0]
//       Header[5] = Z[31:0]
//       Header[6] = DiodeCurrent[31:0] (inserted by software)
//       Header[7] = FrameType[31:0]

namespace Pds {
  namespace Pgp {

    class FirstWordBits {
      public:
        unsigned vc:       2;    // 1:0
        unsigned z:        4;    // 5:2
        unsigned lane:     2;    // 7:6
        unsigned tid:     24;    //31:8
    };

    class SecondWordBits {
      public:
        unsigned acqCount:16;    //15:0
        unsigned opCode:   8;    //23:16
        unsigned quad:     2;    //25:24
        unsigned z:        6;    //31:26
    };

    class DataImportFrame {
      public:
        DataImportFrame() {};
        ~DataImportFrame() {};

      public:
        unsigned portNumber()        { return second.quad; }
        unsigned frameNumber()       { return (unsigned) _frameNumber; }
        unsigned acqCount()          { return second.acqCount; }
        uint32_t ticks()             { return _ticks; }
        uint32_t fiducials()         { return _fiducials; }
        uint32_t frameType()         { return _frameType; }

      public:
        FirstWordBits         first;         // 0
        SecondWordBits        second;        // 1
        uint32_t              _frameNumber;   // 2
        uint32_t              _ticks;
        uint32_t              _fiducials;
        uint16_t              _sbtemp[4];
        uint32_t              _frameType;     // 7
    };

  }
}

#endif /* DATAIMPORTFRAME_HH_ */
