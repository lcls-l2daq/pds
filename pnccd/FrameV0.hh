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

    class ConfigV2;
    class FrameV0 {
      public:
        enum {Version=0};

        uint32_t specialWord();
        uint32_t frameNumber();
        uint32_t timeStampHi();
        uint32_t timeStampLo();
        uint32_t elementId();
        void     elementId(uint32_t);
        void     convertThisToFrameV1();

        uint16_t* data();
        FrameV0* next(ConfigV2& cfg);
        unsigned sizeofData(ConfigV2& cfg);

      private:
        uint32_t _specialWord;
        uint32_t _frameNumber;
        uint32_t _timeStampHi;
        uint32_t _timeStampLo;
    };
  }
}

#endif /* FRAMEV2_HH_ */
