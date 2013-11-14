/*
 * FrameV0.hh
 *
 *  Created on: Nov 6, 2009
 *      Author: jackp
 */

#ifndef FRAMEV0_HH_
#define FRAMEV0_HH_

#include <stdint.h>

namespace Pds {
  namespace PNCCD {

    class Camex {
    public:
      enum {NumChan=128};
      uint16_t data[NumChan];
    };

    class Line {
    public:
      enum {NumCamex=4};
      Camex cmx[NumCamex];
    }; 

    class ImageQuadrant {
    public:
      enum {NumLines=512};
      Line line[NumLines];
    };

    class ConfigV2;
    class FrameV0 {
      public:
        enum {Version=0};

        uint32_t specialWord() const;
        uint32_t frameNumber() const;
        uint32_t timeStampHi() const;
        uint32_t timeStampLo() const;
        uint32_t elementId()   const;
        void     elementId(uint32_t);
        void     convertThisToFrameV1();

        const uint16_t* data() const;
        FrameV0* next(const ConfigV2& cfg) const;
        unsigned sizeofData(ConfigV2& cfg) const;

        void shuffle(void* out) const;
      private:
        uint32_t _specialWord;
        uint32_t _frameNumber;
        uint32_t _timeStampHi;
        uint32_t _timeStampLo;
    };
  }
}

#endif /* FRAMEV2_HH_ */
