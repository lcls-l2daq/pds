/*
 * Processor.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXSAMPLER_PROCESSOR_HH_
#define EPIXSAMPLER_PROCESSOR_HH_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pds/service/Routine.hh"

namespace Pds {

  namespace EpixSampler {

    enum {numbEntriesInputGroup=9, numbEntriesOutputGroup=8};
    enum {numberOfInputColumns=256, numberOfGroups=numberOfInputColumns/numbEntriesOutputGroup, numberOfRows=1024};
    enum {numberOfOutputColumns=256};
    //enum {numberOfRows=1};
    enum {numberOfInputBlocks=4, numberOfOutputBlocks=4};
    enum {numberOfFrames=16};

    class EpixSamplerInGroup {
      public:
        EpixSamplerInGroup() {}
        ~EpixSamplerInGroup() {}
      public:
        void clear() {
          for (int i=0; i<numbEntriesInputGroup; i++) inputWords[i]=0x1111*i;
        }
      public:
        uint16_t inputWords[numbEntriesInputGroup];
    };

    class EpixSamplerInRow {
      public:
        EpixSamplerInRow() {}
        ~EpixSamplerInRow() {}
      public:
        void clear(int u) {
          for (int i=0; i<numberOfGroups; i++) groups[i].clear();
          rowNumber = (uint16_t) u & 0xffff;
        }
      public:
        uint16_t rowNumber;
        EpixSamplerInGroup groups[numberOfGroups];
    };

    class EpixSamplerInBlock {
      public:
        EpixSamplerInBlock() {}
        ~EpixSamplerInBlock() {}
      public:
        void clear() {
          for (int i=0; i<numberOfRows; i++) rows[i].clear(i);
        }
      public:
        EpixSamplerInRow rows[numberOfRows];
    };

    class EpixSamplerInFrame {
      public:
        EpixSamplerInFrame() {}
        ~EpixSamplerInFrame() {}
      public:
        void clear() {
          for (int i=0; i<numberOfInputBlocks; i++) blocks[i].clear();
        }
      public:
        EpixSamplerInBlock blocks[numberOfInputBlocks];
    };

    class EpixSamplerOutRow {
      public:
        EpixSamplerOutRow() {}
        ~EpixSamplerOutRow() {}
      public:
        uint32_t columns[numberOfOutputColumns];
    };

    class EpixSamplerOutBlock {
      public:
        EpixSamplerOutBlock() {}
        ~EpixSamplerOutBlock() {}
      public:
        EpixSamplerOutRow rows[numberOfRows];
    };

    class EpixSamplerOutFrame {
      public:
        EpixSamplerOutFrame() {}
//        ~EpixSamplerOutFrame() {}
      public:
        EpixSamplerOutBlock blocks[numberOfOutputBlocks];
    };


    class Processor : public Routine {
      public:
        Processor(unsigned nt, unsigned ii) : _numbThreads(nt), _ioIndex(ii) {};
        virtual ~Processor();

      public:
         unsigned _numbThreads;
         unsigned _ioIndex;

      public:
         void routine(void);

      private:
         static EpixSamplerInFrame _testData[numberOfFrames];
         static EpixSamplerOutFrame _destData[numberOfFrames];
    };

  }

}

#endif /* EPIXSAMPLER_PROCESSOR_HH_ */
