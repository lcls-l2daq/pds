/*
 * Processor.hh
 *
 *  Created on: Mar 22, 2011
 *      Author: jackp
 */

#ifndef FEXAMP_PROCESSOR_HH_
#define FEXAMP_PROCESSOR_HH_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pds/service/Routine.hh"

namespace Pds {

  namespace Fexamp {

    enum {numbEntriesInputGroup=9, numbEntriesOutputGroup=8};
    enum {numberOfInputColumns=256, numberOfGroups=numberOfInputColumns/numbEntriesOutputGroup, numberOfRows=1024};
    enum {numberOfOutputColumns=256};
    //enum {numberOfRows=1};
    enum {numberOfInputBlocks=4, numberOfOutputBlocks=4};
    enum {numberOfFrames=16};

    class FexampInGroup {
      public:
        FexampInGroup() {}
        ~FexampInGroup() {}
      public:
        void clear() {
          for (int i=0; i<numbEntriesInputGroup; i++) inputWords[i]=0x1111*i;
        }
      public:
        uint16_t inputWords[numbEntriesInputGroup];
    };

    class FexampInRow {
      public:
        FexampInRow() {}
        ~FexampInRow() {}
      public:
        void clear(int u) {
          for (int i=0; i<numberOfGroups; i++) groups[i].clear();
          rowNumber = (uint16_t) u & 0xffff;
        }
      public:
        uint16_t rowNumber;
        FexampInGroup groups[numberOfGroups];
    };

    class FexampInBlock {
      public:
        FexampInBlock() {}
        ~FexampInBlock() {}
      public:
        void clear() {
          for (int i=0; i<numberOfRows; i++) rows[i].clear(i);
        }
      public:
        FexampInRow rows[numberOfRows];
    };

    class FexampInFrame {
      public:
        FexampInFrame() {}
        ~FexampInFrame() {}
      public:
        void clear() {
          for (int i=0; i<numberOfInputBlocks; i++) blocks[i].clear();
        }
      public:
        FexampInBlock blocks[numberOfInputBlocks];
    };

    class FexampOutRow {
      public:
        FexampOutRow() {}
        ~FexampOutRow() {}
      public:
        uint32_t columns[numberOfOutputColumns];
    };

    class FexampOutBlock {
      public:
        FexampOutBlock() {}
        ~FexampOutBlock() {}
      public:
        FexampOutRow rows[numberOfRows];
    };

    class FexampOutFrame {
      public:
        FexampOutFrame() {}
//        ~FexampOutFrame() {}
      public:
        FexampOutBlock blocks[numberOfOutputBlocks];
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
         static FexampInFrame _testData[numberOfFrames];
         static FexampOutFrame _destData[numberOfFrames];
    };

  }

}

#endif /* FEXAMP_PROCESSOR_HH_ */
