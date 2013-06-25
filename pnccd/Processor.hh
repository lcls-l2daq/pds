/*
 * Processor.hh
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#ifndef PNCCD_PROCESSOR_HH_
#define PNCCD_PROCESSOR_HH_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pds/service/Routine.hh"

namespace Pds {

  namespace pnCCD {

//    enum {numbEntriesInputGroup=9, numbEntriesOutputGroup=8};
//    enum {numberOfInputColumns=256, numberOfGroups=numberOfInputColumns/numbEntriesOutputGroup, numberOfRows=1024};
//    enum {numberOfOutputColumns=256};
//    //enum {numberOfRows=1};
//    enum {numberOfInputBlocks=4, numberOfOutputBlocks=4};
//    enum {numberOfFrames=16};

//    class pnCCDInGroup {
//      public:
//        pnCCDInGroup() {}
//        ~pnCCDInGroup() {}
//      public:
//        void clear() {
//          for (int i=0; i<numbEntriesInputGroup; i++) inputWords[i]=0x1111*i;
//        }
//      public:
//        uint16_t inputWords[numbEntriesInputGroup];
//    };
//
//    class pnCCDInRow {
//      public:
//        pnCCDInRow() {}
//        ~pnCCDInRow() {}
//      public:
//        void clear(int u) {
//          for (int i=0; i<numberOfGroups; i++) groups[i].clear();
//          rowNumber = (uint16_t) u & 0xffff;
//        }
//      public:
//        uint16_t rowNumber;
//        pnCCDInGroup groups[numberOfGroups];
//    };
//
//    class pnCCDInBlock {
//      public:
//        pnCCDInBlock() {}
//        ~pnCCDInBlock() {}
//      public:
//        void clear() {
//          for (int i=0; i<numberOfRows; i++) rows[i].clear(i);
//        }
//      public:
//        pnCCDInRow rows[numberOfRows];
//    };
//
//    class pnCCDInFrame {
//      public:
//        pnCCDInFrame() {}
//        ~pnCCDInFrame() {}
//      public:
//        void clear() {
//          for (int i=0; i<numberOfInputBlocks; i++) blocks[i].clear();
//        }
//      public:
//        pnCCDInBlock blocks[numberOfInputBlocks];
//    };
//
//    class pnCCDOutRow {
//      public:
//        pnCCDOutRow() {}
//        ~pnCCDOutRow() {}
//      public:
//        uint32_t columns[numberOfOutputColumns];
//    };
//
//    class pnCCDOutBlock {
//      public:
//        pnCCDOutBlock() {}
//        ~pnCCDOutBlock() {}
//      public:
//        pnCCDOutRow rows[numberOfRows];
//    };
//
//    class pnCCDOutFrame {
//      public:
//        pnCCDOutFrame() {}
////        ~pnCCDOutFrame() {}
//      public:
//        pnCCDOutBlock blocks[numberOfOutputBlocks];
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
//         static pnCCDInFrame _testData[numberOfFrames];
//         static pnCCDOutFrame _destData[numberOfFrames];
    };

  }

}

#endif /* PNCCD_PROCESSOR_HH_ */
