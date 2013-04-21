/*
 * Processor.hh
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#ifndef IMP_PROCESSOR_HH_
#define IMP_PROCESSOR_HH_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pds/service/Routine.hh"

namespace Pds {

  namespace Imp {

//    enum {numbEntriesInputGroup=9, numbEntriesOutputGroup=8};
//    enum {numberOfInputColumns=256, numberOfGroups=numberOfInputColumns/numbEntriesOutputGroup, numberOfRows=1024};
//    enum {numberOfOutputColumns=256};
//    //enum {numberOfRows=1};
//    enum {numberOfInputBlocks=4, numberOfOutputBlocks=4};
//    enum {numberOfFrames=16};

//    class ImpInGroup {
//      public:
//        ImpInGroup() {}
//        ~ImpInGroup() {}
//      public:
//        void clear() {
//          for (int i=0; i<numbEntriesInputGroup; i++) inputWords[i]=0x1111*i;
//        }
//      public:
//        uint16_t inputWords[numbEntriesInputGroup];
//    };
//
//    class ImpInRow {
//      public:
//        ImpInRow() {}
//        ~ImpInRow() {}
//      public:
//        void clear(int u) {
//          for (int i=0; i<numberOfGroups; i++) groups[i].clear();
//          rowNumber = (uint16_t) u & 0xffff;
//        }
//      public:
//        uint16_t rowNumber;
//        ImpInGroup groups[numberOfGroups];
//    };
//
//    class ImpInBlock {
//      public:
//        ImpInBlock() {}
//        ~ImpInBlock() {}
//      public:
//        void clear() {
//          for (int i=0; i<numberOfRows; i++) rows[i].clear(i);
//        }
//      public:
//        ImpInRow rows[numberOfRows];
//    };
//
//    class ImpInFrame {
//      public:
//        ImpInFrame() {}
//        ~ImpInFrame() {}
//      public:
//        void clear() {
//          for (int i=0; i<numberOfInputBlocks; i++) blocks[i].clear();
//        }
//      public:
//        ImpInBlock blocks[numberOfInputBlocks];
//    };
//
//    class ImpOutRow {
//      public:
//        ImpOutRow() {}
//        ~ImpOutRow() {}
//      public:
//        uint32_t columns[numberOfOutputColumns];
//    };
//
//    class ImpOutBlock {
//      public:
//        ImpOutBlock() {}
//        ~ImpOutBlock() {}
//      public:
//        ImpOutRow rows[numberOfRows];
//    };
//
//    class ImpOutFrame {
//      public:
//        ImpOutFrame() {}
////        ~ImpOutFrame() {}
//      public:
//        ImpOutBlock blocks[numberOfOutputBlocks];
//    };


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
//         static ImpInFrame _testData[numberOfFrames];
//         static ImpOutFrame _destData[numberOfFrames];
    };

  }

}

#endif /* IMP_PROCESSOR_HH_ */
