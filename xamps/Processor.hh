/*
 * Processor.hh
 *
 *  Created on: Mar 22, 2011
 *      Author: jackp
 */

#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pds/service/Routine.hh"

namespace Pds {

  namespace Xamps {

    enum {numbEntriesInputGroup=9, numbEntriesOutputGroup=8};
    enum {numberOfInputColumns=256, numberOfGroups=numberOfInputColumns/numbEntriesOutputGroup, numberOfRows=1024};
    enum {numberOfOutputColumns=256};
    //enum {numberOfRows=1};
    enum {numberOfInputBlocks=4, numberOfOutputBlocks=4};
    enum {numberOfFrames=16};

    class XampsInGroup {
      public:
        XampsInGroup() {}
        ~XampsInGroup() {}
      public:
        void clear() {
          for (int i=0; i<numbEntriesInputGroup; i++) inputWords[i]=0x1111*i;
        }
      public:
        uint16_t inputWords[numbEntriesInputGroup];
    };

    class XampsInRow {
      public:
        XampsInRow() {}
        ~XampsInRow() {}
      public:
        void clear(int u) {
          for (int i=0; i<numberOfGroups; i++) groups[i].clear();
          rowNumber = (uint16_t) u & 0xffff;
        }
      public:
        uint16_t rowNumber;
        XampsInGroup groups[numberOfGroups];
    };

    class XampsInBlock {
      public:
        XampsInBlock() {}
        ~XampsInBlock() {}
      public:
        void clear() {
          for (int i=0; i<numberOfRows; i++) rows[i].clear(i);
        }
      public:
        XampsInRow rows[numberOfRows];
    };

    class XampsInFrame {
      public:
        XampsInFrame() {}
        ~XampsInFrame() {}
      public:
        void clear() {
          for (int i=0; i<numberOfInputBlocks; i++) blocks[i].clear();
        }
      public:
        XampsInBlock blocks[numberOfInputBlocks];
    };

    class XampsOutRow {
      public:
        XampsOutRow() {}
        ~XampsOutRow() {}
      public:
        uint32_t columns[numberOfOutputColumns];
    };

    class XampsOutBlock {
      public:
        XampsOutBlock() {}
        ~XampsOutBlock() {}
      public:
        XampsOutRow rows[numberOfRows];
    };

    class XampsOutFrame {
      public:
        XampsOutFrame() {}
//        ~XampsOutFrame() {}
      public:
        XampsOutBlock blocks[numberOfOutputBlocks];
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
         static XampsInFrame _testData[numberOfFrames];
         static XampsOutFrame _destData[numberOfFrames];
    };

  }

}

#endif /* PROCESSOR_HH_ */
